package ru.yandex.ci.flow.engine.runtime;

import java.util.NoSuchElementException;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.job.JobInstanceSource;
import ru.yandex.ci.core.job.TaskUnrecoverableException;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.SyncJobExecutor.State;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorFailedToInterruptEvent;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptedEvent;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptingEvent;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorKilledEvent;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.GenerateJobsEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorExpectedFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExpectedFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobForceSuccessEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobInterruptedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobKilledEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.exceptions.FlowDisabledException;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobDisabledException;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobManualFailException;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.zookeeper.CuratorFactory;
import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.tasklet.api.v2.DataModel;

@Slf4j
@RequiredArgsConstructor
public class JobLauncher {

    @Nonnull
    private final CuratorFactory curatorFactory;
    @Nonnull
    private final FlowStateService flowStateService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final JobContextFactory jobContextFactory;
    @Nonnull
    private final JobExecutorProvider jobExecutorProvider;
    @Nonnull
    private final JobConditionalChecks jobConditionalChecks;
    @Nonnull
    private final JobResourcesValidator jobResourcesValidator;

    /**
     * Пересчитывает флоу с учётом того, что джоба запущена, исполняет джобу и пересчитывает флоу с учётом того,
     * что джоба завершилась.
     *
     * @param jobLaunchId уникальный код запускаемой задачи
     * @param tmsTaskId   id задачи внутренней tms
     */
    public void launchJob(FullJobLaunchId jobLaunchId, TmsTaskId tmsTaskId) {
        var flowLaunchId = jobLaunchId.getFlowLaunchId();
        var flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
        var context = new Context(jobLaunchId, tmsTaskId, flowLaunch);
        try {
            this.launchJobImpl(context);
        } catch (Exception e) {
            if (e instanceof TaskUnrecoverableException) {
                // Some errors could be treated as critical errors, there is no point to repeat task in that case
                log.error("{} failed", jobLaunchId, e);
                var jobId = jobLaunchId.getJobId();
                context.recalc(new JobExecutorFailedEvent(
                        jobId,
                        context.getJobLaunch().getNumber(),
                        ResourceRefContainer.empty(),
                        e
                ));
                notifyJobExecutorFinished(context);
                finish(context);
            } else {
                throw e;
            }
        }
    }

    private void launchJobImpl(Context context) {
        JobLaunch jobLaunch = context.getJobLaunch();
        StatusChange lastStatusChange = jobLaunch.getLastStatusChange();

        // Beware! cases without breaks
        // Task must be traversed through all states in order to be fully executed (with success or fail state)
        switch (lastStatusChange.getType()) {
            case QUEUED:
            case RUNNING:
                tryRunExecutor(context, lastStatusChange.getType() == StatusChangeType.RUNNING);
                // fall through
            case EXECUTOR_FAILED:
            case EXECUTOR_EXPECTED_FAILED:
            case EXECUTOR_SUCCEEDED:
            case EXECUTOR_INTERRUPTED:
            case EXECUTOR_KILLED:
            case FORCED_EXECUTOR_SUCCEEDED:
            case SUBSCRIBERS_FAILED:
            case INTERRUPTING:
                notifyJobExecutorFinished(context);
                // fall through
            case SUBSCRIBERS_SUCCEEDED:
                finish(context);
                // fall through
            case SUCCESSFUL:
            case INTERRUPTED:
            case KILLED:
            case EXPECTED_FAILED:
            case FAILED:
                // finished
                break;
            default:
                // Check if flow was not canceled
                if (checkIfLaunchCanceled(context)) {
                    log.info("Skipping job entirely");
                    return; // --
                }
                throw new IllegalStateException("Unknown job state: " + lastStatusChange.getType());
        }
    }

    private boolean checkIfLaunchCanceled(Context context) {
        var launchId = context.getFlowLaunch().getLaunchId();
        var launch = db.currentOrReadOnly(() ->
                db.launches().findOptional(launchId));
        if (launch.isPresent()) {
            if (launch.get().getStatus() == LaunchState.Status.CANCELED) {
                log.info("Flow launch {} is canceled already, stopping task", launchId);
                return true;
            }
        } else {
            log.error("Unable fo find launch {}", launchId);
        }
        return false;
    }

    private void tryRunExecutor(Context context, boolean recover) {
        FullJobLaunchId fullJobLaunchId = context.getFullJobLaunchId();
        String jobId = fullJobLaunchId.getJobId();
        FlowLaunchEntity flowLaunch = context.getFlowLaunch();
        JobState jobState = flowLaunch.getJobs().get(jobId);
        JobLaunch jobLaunch = context.getJobLaunch();

        log.info("Running fullJobLaunchId {}", fullJobLaunchId);

        if (flowLaunch.isDisabled()) {
            context.recalc(new JobExecutorFailedEvent(
                    jobId,
                    jobLaunch.getNumber(),
                    ResourceRefContainer.empty(),
                    new FlowDisabledException()
            ));
            return; // --- flow disabled, cannot continue
        }

        if (jobState.isDisabled()) {
            context.recalc(new JobExecutorFailedEvent(
                    jobId,
                    jobLaunch.getNumber(),
                    ResourceRefContainer.empty(),
                    new JobDisabledException()
            ));
            return; // --- job disabled, cannot continue
        }

        if (flowLaunch.isCleanupRunning()) { // Have to check if we need to skip this launch
            if (jobState.getJobType() != JobType.CLEANUP) {
                log.info("Skip {} job {} during cleanup state", jobState.getJobType(), jobState.getJobId());
                var event = recover
                        ? new ExecutorInterruptingEvent(jobId, jobLaunch.getNumber(), "internal-cleanup")
                        : new JobForceSuccessEvent(jobId, jobLaunch.getNumber(), context.getTmsTaskId(), true);
                context.recalc(event);
                return; // --- No additional actions required
            }
        }

        var jobContext = jobContextFactory.createJobContext(flowLaunch, jobState);
        if (jobConditionalChecks.shouldRun(jobContext)) {
            if (jobState.getMultiply() == null) {
                if (!recover) {
                    context.recalc(new JobRunningEvent(jobId, jobLaunch.getNumber(), context.getTmsTaskId()));
                }
            } else {
                context.recalc(new GenerateJobsEvent(jobContextFactory, jobId));
                context.recalc(new JobForceSuccessEvent(jobId, jobLaunch.getNumber(), context.getTmsTaskId()));
                return; // --- No additional actions required
            }
        } else {
            context.recalc(new JobForceSuccessEvent(jobId, jobLaunch.getNumber(), context.getTmsTaskId(), true));
            return; // --- No additional actions required
        }

        var executor = jobExecutorProvider.createJobExecutor(jobContext);
        log.info("Using job executor {}", executor);
        runExecutor(context, recover, executor, jobContext);
    }

    private void runExecutor(Context context, boolean recover, JobExecutor executor, JobContext jobContext) {
        try (var syncJobExecutor = new SyncJobExecutor(
                curatorFactory,
                executor,
                jobContext,
                () -> onInterruptFailed(context),
                recover
        )) {
            String jobId = context.getFullJobLaunchId().getJobId();
            log.info("Watching for jobId {}", jobId);

            // All interrupt/kill events will be handled inside executor
            syncJobExecutor.execute();

            // Make sure to reset current `interrupted` status
            boolean threadInterrupted = Thread.interrupted();

            var executionException = syncJobExecutor.getExecutorException();
            if (executionException != null) {
                log.error(
                        "Job executor failed, jobId = {}, flowLaunchId = {}",
                        jobId, context.getFullJobLaunchId().getFlowLaunchId(),
                        executionException
                );
            }

            var state = syncJobExecutor.getState();
            log.info("Job state: {}", state);
            if (state == State.KILLED) {
                onJobKilled(context, syncJobExecutor);
            } else if (state == State.INTERRUPTED && executionException == null) {
                onJobInterrupted(context, syncJobExecutor);
            } else if (executionException != null &&
                    (ExceptionUtils.isCausedBy(executionException, InterruptedException.class) || threadInterrupted)) {
                throw executionException;
            } else {
                onJobComplete(context, executionException, syncJobExecutor);
            }
        } catch (RuntimeException | Error e) {
            throw e;
        } catch (Throwable exception) {
            throw new RuntimeException("Failed to run job executor", exception);
        }
    }

    private void onInterruptFailed(Context context) {
        String jobId = context.getFullJobLaunchId().getJobId();
        int jobLaunchNumber = context.getFullJobLaunchId().getJobLaunchNumber();

        log.info("Job interrupt failed: {}", jobId);

        context.recalc(new ExecutorFailedToInterruptEvent(jobId, jobLaunchNumber));
    }

    private void onJobKilled(Context context, SyncJobExecutor syncJobExecutor) {
        String jobId = context.getFullJobLaunchId().getJobId();
        int jobLaunchNumber = context.getFullJobLaunchId().getJobLaunchNumber();

        log.info("Job killed: {}", jobId);

        context.recalc(new ExecutorKilledEvent(
                jobId,
                jobLaunchNumber,
                syncJobExecutor.getProducedResources().toRefs()
        ));
    }

    private void onJobInterrupted(Context context, SyncJobExecutor syncJobExecutor) {
        String jobId = context.getFullJobLaunchId().getJobId();
        log.info("Job interrupted: {}", jobId);

        int jobLaunchNumber = context.getFullJobLaunchId().getJobLaunchNumber();

        saveResources(context, syncJobExecutor);

        context.recalc(new ExecutorInterruptedEvent(
                jobId,
                jobLaunchNumber,
                syncJobExecutor.getProducedResources().toRefs()
        ));
    }

    private void onJobComplete(
            Context context,
            @Nullable Throwable executionException,
            SyncJobExecutor syncJobExecutor) {

        var jobId = context.getFullJobLaunchId().getJobId();
        log.info("Completing job {}", jobId);

        saveResources(context, syncJobExecutor);

        final FlowEvent event;
        if (executionException == null) {
            event = new JobExecutorSucceededEvent(jobId, context.getJobLaunch().getNumber(),
                    syncJobExecutor.getProducedResources().toRefs());
        } else {
            if (executionException instanceof JobManualFailException) {
                event = new JobExecutorExpectedFailedEvent(
                        jobId,
                        context.getJobLaunch().getNumber(),
                        syncJobExecutor.getProducedResources().toRefs(),
                        ((JobManualFailException) executionException).getSupportInfo(),
                        executionException
                );
            } else {
                SandboxTaskStatus taskStatus;
                DataModel.ErrorCodes.ErrorCode taskletServerError;
                if (executionException instanceof JobInstanceSource source) {
                    var job = source.getJobInstance();
                    taskStatus = job.getSandboxTaskStatus();
                    taskletServerError = job.getTaskletServerError();
                } else {
                    taskStatus = null;
                    taskletServerError = null;
                }
                event = new JobExecutorFailedEvent(
                        jobId,
                        context.getJobLaunch().getNumber(),
                        syncJobExecutor.getProducedResources().toRefs(),
                        executionException,
                        taskStatus,
                        taskletServerError
                );
            }
        }
        context.recalc(event);
    }

    private void notifyJobExecutorFinished(Context context) {
        FullJobLaunchId fullJobLaunchId = context.getFullJobLaunchId();
        String jobId = fullJobLaunchId.getJobId();
        JobState jobState = context.getFlowLaunch().getJobs().get(jobId);

        if (context.getFlowLaunch().isDisabled()) {
            context.recalc(
                    new SubscribersFailedEvent(
                            fullJobLaunchId.getJobId(), fullJobLaunchId.getJobLaunchNumber(),
                            new FlowDisabledException()
                    )
            );
            return;
        }

        if (jobState.isDisabled()) {
            context.recalc(
                    new SubscribersFailedEvent(
                            fullJobLaunchId.getJobId(), fullJobLaunchId.getJobLaunchNumber(), new JobDisabledException()
                    )
            );
            return;
        }

        int jobLaunchNumber = fullJobLaunchId.getJobLaunchNumber();
        context.recalc(new SubscribersSucceededEvent(jobId, jobLaunchNumber));
    }

    private void saveResources(Context context, SyncJobExecutor syncJobExecutor) {
        var flowLaunch = context.getFlowLaunch();
        jobResourcesValidator.validateAndSaveResources(flowLaunch.getProjectId(), flowLaunch.getFlowLaunchId(),
                syncJobExecutor.getProducedResources());
    }

    private void finish(Context context) {
        String jobId = context.getFullJobLaunchId().getJobId();
        JobLaunch jobLaunch = context.getJobLaunch();

        boolean didExecutorTryToInterrupt = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.INTERRUPTING);

        boolean hasExecutorInterrupted = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.EXECUTOR_INTERRUPTED);

        boolean hasExecutorSucceeded = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.EXECUTOR_SUCCEEDED);

        boolean hasExecutorFailed = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.EXECUTOR_FAILED);

        boolean hasExecutorExpectedFailed = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.EXECUTOR_EXPECTED_FAILED);

        boolean hasForcedExecutorSucceeded = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.FORCED_EXECUTOR_SUCCEEDED);

        boolean hasSubscribersSucceeded = jobLaunch.getStatusHistory().stream()
                .anyMatch(s -> s.getType() == StatusChangeType.SUBSCRIBERS_SUCCEEDED);

        if (hasSubscribersSucceeded) {
            JobEvent jobEvent;
            if (hasExecutorSucceeded || hasForcedExecutorSucceeded) {
                jobEvent = new JobSucceededEvent(jobId, jobLaunch.getNumber());
            } else if (hasExecutorFailed) {
                jobEvent = new JobFailedEvent(jobId, jobLaunch.getNumber());
            } else if (hasExecutorExpectedFailed) {
                jobEvent = new JobExpectedFailedEvent(jobId, jobLaunch.getNumber());
            } else if (didExecutorTryToInterrupt) {
                jobEvent = hasExecutorInterrupted ?
                        new JobInterruptedEvent(jobId, jobLaunch.getNumber()) :
                        new JobKilledEvent(jobId, jobLaunch.getNumber());
            } else {
                throw new RuntimeException("Unknown state");
            }

            context.recalc(jobEvent);
        }
    }

    class Context {
        private static final int MAX_RECALC_ATTEMPTS = 3;

        private final FullJobLaunchId fullJobLaunchId;
        private final TmsTaskId tmsTaskId;

        // It could be updated from different thread, see #onInterruptFailed
        private volatile FlowLaunchEntity flowLaunch;

        Context(FullJobLaunchId fullJobLaunchId, TmsTaskId tmsTaskId, FlowLaunchEntity flowLaunch) {
            this.fullJobLaunchId = fullJobLaunchId;
            this.tmsTaskId = tmsTaskId;
            this.flowLaunch = flowLaunch;
        }

        FullJobLaunchId getFullJobLaunchId() {
            return fullJobLaunchId;
        }

        TmsTaskId getTmsTaskId() {
            return tmsTaskId;
        }

        FlowLaunchEntity getFlowLaunch() {
            return flowLaunch;
        }

        JobLaunch getJobLaunch() {
            JobState jobState = flowLaunch.getJobs().get(fullJobLaunchId.getJobId());
            return jobState.getLaunches()
                    .stream()
                    .filter(l -> l.getNumber() == fullJobLaunchId.getJobLaunchNumber())
                    .findFirst()
                    .orElseThrow(() -> new NoSuchElementException("Unable to find job launch " + fullJobLaunchId));
        }

        void recalc(FlowEvent event) {
            for (int attempt = 1; true; ++attempt) {
                try {
                    this.flowLaunch = flowStateService.recalc(fullJobLaunchId.getFlowLaunchId(), event);
                    return;
                } catch (Exception e) {
                    if (attempt >= MAX_RECALC_ATTEMPTS) {
                        throw e;
                    }
                    log.warn("Failed to recalc flow launch {} after event {}",
                            flowLaunch.getIdString(), event, e);
                    try {
                        // Wait for concurrency changes. https://st.yandex-team.ru/MARKETINFRA-4353
                        //noinspection BusyWait
                        Thread.sleep(1);
                    } catch (InterruptedException interruptedException) {
                        throw new RuntimeException("Interrupted", interruptedException);
                    }
                }
            }
        }
    }
}
