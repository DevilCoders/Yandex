package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonPrimitive;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.resolver.DocumentSource;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.definition.job.AbstractJob;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.runtime.ExecutorType;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.events.InvalidateLaunchEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobLaunchEvent;
import ru.yandex.ci.flow.engine.runtime.events.ScheduleChangeEvent;
import ru.yandex.ci.flow.engine.runtime.events.StageGroupChangeEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.exceptions.FlowDisabledException;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobDisabledException;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.model.UpstreamLaunch;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;

/**
 * Рантайм-модель джобы. Здесь происходит пересчёт состояния джобы
 * в зависимости от изменений в апстримах или внешних событий (@see {@link JobEvent}).
 */
public class JobRuntime implements AbstractJob<JobRuntime> {
    private static final Logger log = LogManager.getLogger();

    private final Set<UpstreamLink<JobRuntime>> upstreams;
    private final JobState jobState;
    private final FlowLaunchEntity flowLaunch;
    @Nullable
    private final JobExecutorObject executorObject;
    private final ResourceProvider resourceProvider;
    private final UpstreamResourcesCollector upstreamResourcesCollector;
    private final StageGroupState stageGroupState;

    private JobStatus computedStatus;

    JobRuntime(
            Set<UpstreamLink<JobRuntime>> upstreams,
            JobState jobState,
            FlowLaunchEntity flowLaunch,
            @Nullable JobExecutorObject executorObject,
            ResourceProvider resourceProvider,
            UpstreamResourcesCollector upstreamResourcesCollector,
            StageGroupState stageGroupState
    ) {
        this.upstreams = upstreams;
        this.jobState = jobState;
        this.flowLaunch = flowLaunch;
        this.executorObject = executorObject;
        this.resourceProvider = resourceProvider;
        this.upstreamResourcesCollector = upstreamResourcesCollector;
        this.stageGroupState = stageGroupState;
    }

    @Override
    public String getId() {
        return jobState.getJobId();
    }

    JobStatus getStatus() {
        return this.computedStatus;
    }

    JobState getJobState() {
        return jobState;
    }

    JobLaunch getLastLaunch() {
        return jobState.getLastLaunch();
    }

    @Override
    public Set<UpstreamLink<JobRuntime>> getUpstreams() {
        return upstreams;
    }

    void processEvent(PendingCommandConsumer commandConsumer, @Nullable FlowEvent event) {
        if (this.computedStatus != null) {
            return;
        }

        for (UpstreamLink<JobRuntime> upstream : upstreams) {
            upstream.getEntity().processEvent(commandConsumer, event);
        }

        if (event instanceof StageGroupChangeEvent) {
            maybeTriggerIfWaitingForStage(commandConsumer);
        } else if (event instanceof InvalidateLaunchEvent &&
                jobState.getLastLaunch() != null &&
                jobState.getLastStatusChangeType() == StatusChangeType.QUEUED) {
            trigger(commandConsumer, null, false);
        } else if (event instanceof ScheduleChangeEvent
                && this.getId().equals(((ScheduleChangeEvent) event).getJobId())) {
            maybeTriggerIfWaitingForSchedule(commandConsumer);
        } else if (event instanceof JobEvent jobEvent && this.getId().equals(((JobEvent) event).getJobId())) {
            JobLaunch launch = jobState.getLastLaunch();
            if (event instanceof JobLaunchEvent) {
                launch = jobState.getLaunchByNumber(((JobLaunchEvent) event).getJobLaunchNumber());
            }

            if (launch != null) {
                StatusChangeType currentStatus = launch.getLastStatusChangeType();

                runStatusChangePreChecks(jobEvent, currentStatus);

                StatusChange nextStatusChange = jobEvent.createNextStatusChange();
                if (nextStatusChange != null) {
                    log.info("Job [{}] transition: {} -> {}",
                            jobState.getJobId(), currentStatus, nextStatusChange.getType());
                    launch.recordStatusChange(nextStatusChange);
                }
            }

            jobEvent.afterStatusChange(launch, jobState);

            processJobEventCustomActions(commandConsumer, jobEvent);
        }

        this.setComputedStatus(computeStatus());

        if (!(event instanceof ForceSuccessTriggerEvent)) {
            boolean needWaitSchedule = jobState.isEnableJobSchedulerConstraint()
                    && jobState.getJobSchedulerConstraint() != null
                    && !(event instanceof ScheduleChangeEvent);

            maybeTrigger(commandConsumer, needWaitSchedule);
        }
    }

    private void processJobEventCustomActions(PendingCommandConsumer commandConsumer, JobEvent event) {
        if (event instanceof ForceSuccessTriggerEvent) {
            commandConsumer.scheduleJob(jobState, flowLaunch.getFlowLaunchId().asString());
        } else if (event instanceof TriggerEvent triggerEvent) {
            if (jobState.getLastLaunch() != null) {
                if (triggerEvent.shouldRestartIfAlreadyRunning()) {
                    if (jobState.getLastStatusChangeType() == StatusChangeType.RUNNING) {
                        interruptJobLaunch(
                                commandConsumer,
                                triggerEvent.getTriggeredBy(),
                                jobState,
                                jobState.getLastLaunch()
                        );
                    }
                } else {
                    if (!jobState.getLastLaunch().getLastStatusChangeType().isFinished()
                            && !jobState.isWaitingForScheduleChangeType()) {
                        log.info("Job {} is already running, cannot trigger again", jobState.getJobId());
                        return;
                    }
                }
            }
            trigger(commandConsumer, triggerEvent.getTriggeredBy(), false);
            interruptRunningDownstreams(commandConsumer, triggerEvent);
        } else if (event instanceof JobFailedEvent &&
                jobState.getRetries() > 0 && !jobState.isDisabled()) {
            // TODO: Почему lastLaunch, а не JobFailedEvent.getLaunchNumber()?
            var launch = jobState.getLastLaunch();
            if (launch != null) {
                if (acceptRetryJob(launch, ExecutorType.selectFor(jobState.getExecutorContext()))) {
                    jobState.setRetries(jobState.getRetries() - 1);
                    log.info("Retrying job {}, remaining retries {}", jobState.getJobId(), jobState.getRetries());
                    trigger(commandConsumer, launch.getTriggeredBy(), needWaitSchedule(jobState.getRetry()));
                }
            }
        }
    }

    private boolean acceptRetryJob(JobLaunch launch, ExecutorType executorType) {
        var retryConfig = jobState.getRetry();

        var sandboxConfig = retryConfig.getSandboxConfig();
        if (sandboxConfig != null) {
            var sandboxTaskStatus = launch.getSandboxTaskStatus();
            if (sandboxConfig.getExcludeStatuses().contains(sandboxTaskStatus)) {
                log.info("Ignore Sandbox status {} due to attempts settings: {}",
                        sandboxTaskStatus, sandboxConfig);
                return false;
            }
        }

        if (executorType == ExecutorType.TASKLET_V2) {
            var taskletServerError = launch.getTaskletServerError();
            if (taskletServerError == null) {
                log.info("Do not restart Tasklet V2 without ServerError");
                return false;
            }
            var taskletConfig = retryConfig.getTaskletConfig();
            if (taskletConfig != null) {
                if (taskletConfig.getExcludeServerErrors().contains(taskletServerError)) {
                    log.info("Ignore Tasklet V2 server error {} due to attempts settings: {}",
                            taskletServerError, taskletConfig);
                    return false;
                }
            }
        }

        var expression = retryConfig.getIfOutput();
        if (expression != null) {
            log.info("Job [{}] must be checked for conditional retry: {}", getId(), expression);
            try {
                var result = PropertiesSubstitutor.substitute(
                        new JsonPrimitive(retryConfig.getIfOutput()),
                        DocumentSource.of(() -> {
                            var json = upstreamResourcesCollector.collectResource(flowLaunch, jobState, launch);
                            var flowVars = flowLaunch.getFlowInfo().getFlowVars();
                            if (flowVars != null) {
                                json.add(PropertiesSubstitutor.FLOW_VARS_KEY, flowVars.getData());
                            }
                            return json;
                        })
                );
                var shouldRetry = PropertiesSubstitutor.asBoolean(result);
                log.info("Job [{}] retry evaluated, should retry = {}, based on result [{}]",
                        getId(), shouldRetry, result);
                return shouldRetry;
            } catch (Exception e) {
                log.warn("Unable to resolve conditional retry", e);
            }
        }
        return true;
    }

    private static boolean needWaitSchedule(@Nullable JobAttemptsConfig retry) {
        return retry != null && retry.needWaitSchedule();
    }

    private void interruptRunningDownstreams(PendingCommandConsumer commandConsumer, TriggerEvent event) {
        for (JobState downstream : flowLaunch.getDownstreamsRecursive(event.getJobId())) {
            interruptRunningJobLaunches(commandConsumer, event.getTriggeredBy(), downstream);
        }
    }

    private void interruptRunningJobLaunches(PendingCommandConsumer commandConsumer, String interruptedBy,
                                             JobState jobToInterrupt) {
        for (JobLaunch jobLaunch : jobToInterrupt.getLaunches()) {
            if (jobLaunch.getLastStatusChangeType() == StatusChangeType.RUNNING) {
                interruptJobLaunch(commandConsumer, interruptedBy, jobToInterrupt, jobLaunch);
            }
        }
    }

    private void interruptJobLaunch(PendingCommandConsumer commandConsumer, String interruptedBy,
                                    JobState jobToInterrupt, JobLaunch jobLaunchToInterrupt) {
        jobLaunchToInterrupt.recordStatusChange(StatusChange.interrupting());
        jobLaunchToInterrupt.setInterruptedBy(interruptedBy);

        var id = new FullJobLaunchId(
                flowLaunch.getIdString(),
                jobToInterrupt.getJobId(),
                jobLaunchToInterrupt.getNumber()
        );
        log.info("Interrupting {}", id);
        commandConsumer.interruptJob(id);
    }

    private void runStatusChangePreChecks(JobEvent jobEvent, StatusChangeType status) {
        Preconditions.checkState(
                jobEvent.doesSupportStatus(status),
                "Event %s does not support status %s", jobEvent.getClass().getSimpleName(), status
        );

        if (!jobEvent.doesSupportDisabledJobs()) {
            ensureNotDisabled();
        }

        jobEvent.checkJobIsInValidState(jobState);
    }

    private void ensureNotDisabled() {
        ensureFlowNotDisabledOrDisabling();
        ensureJobNotDisabled();
    }

    private void ensureFlowNotDisabledOrDisabling() {
        if (flowLaunch.isDisabled()
                || (flowLaunch.isDisablingGracefully() && flowLaunch.shouldIgnoreUninterruptibleStages())) {
            throw new FlowDisabledException();
        }
    }

    private void ensureJobNotDisabled() {
        if (this.jobState.isDisabled()) {
            throw new JobDisabledException();
        }
    }

    private void checkUpstreamNoEmpty(List<JobStatus> upstreamStatuses) {
        if (upstreamStatuses.isEmpty()) {
            throw new IllegalStateException(String.format(
                    "job %s has zero upstreams and configured to wait at least one of them." +
                            " Job %s will never be scheduled. Check the configuration",
                    jobState.getJobId(), jobState.getJobId()
            ));
        }
    }

    private JobStatus computeStatus() {
        var upstreamStatuses = getUpstreamStatuses();

        boolean isReadyToRun = switch (jobState.getCanRunWhen()) {
            case ANY_COMPLETED -> {
                checkUpstreamNoEmpty(upstreamStatuses);
                yield upstreamStatuses.stream()
                        .anyMatch(upstreamStatus ->
                                !upstreamStatus.isOutdated() &&
                                        upstreamStatus.getStatusChange() != null &&
                                        upstreamStatus.getStatusChange().getType() == StatusChangeType.SUCCESSFUL);
            }
            case ANY_FAILED -> {
                checkUpstreamNoEmpty(upstreamStatuses);
                yield upstreamStatuses.stream()
                        .anyMatch(upstreamStatus ->
                                !upstreamStatus.isOutdated() &&
                                        upstreamStatus.getStatusChange() != null &&
                                        upstreamStatus.getStatusChange().getType() == StatusChangeType.FAILED);
            }
            case ALL_COMPLETED -> upstreamStatuses.stream()
                    .allMatch(upstreamStatus ->
                            !upstreamStatus.isOutdated() &&
                                    upstreamStatus.getStatusChange() != null &&
                                    upstreamStatus.getStatusChange().getType() == StatusChangeType.SUCCESSFUL
                    );
        };

        isReadyToRun = isReadyToRun && isStagedJobReadyToRun();

        JobLaunch lastLaunch = jobState.getLastLaunch();
        if (lastLaunch == null) {
            return JobStatus.noLaunches(isReadyToRun);
        }

        StatusChange lastStatusChange = lastLaunch.getLastStatusChange();
        if (!isUpToDate(lastLaunch)) {
            return JobStatus.outdated(lastStatusChange, isReadyToRun);
        }

        return JobStatus.actual(lastStatusChange, isReadyToRun);
    }

    private boolean isStagedJobReadyToRun() {
        return
                // not staged job or stage acquired
                (jobState.getStage() == null || isStageAcquired(jobState.getStage())) ||

                        // first job without launches can acquire first stage
                        (jobState.getUpstreams().isEmpty() && jobState.getLaunches().isEmpty()) ||

                        // job on new stage can acquire stage
                        (upstreams.stream()
                                .map(UpstreamLink::getEntity)
                                .anyMatch(x -> isStageAcquired(x.jobState.getStage())) &&
                                jobState.getLaunches().isEmpty());
    }

    private void maybeTrigger(PendingCommandConsumer commandConsumer, boolean needWaitSchedule) {
        if (flowLaunch.isDisablingGracefully()) {
            if (stageGroupState == null || jobState.getStage() == null) {
                return;
            }

            boolean stageCanBeInterrupted = flowLaunch.getStage(jobState.getStage()).isCanBeInterrupted();
            boolean interruptStageAnyway = flowLaunch.shouldIgnoreUninterruptibleStages();

            if (stageCanBeInterrupted || interruptStageAnyway) {
                return;
            }
        }

        if (this.computedStatus.isReadyToRun()
                && !this.jobState.isDisabled()
                && (this.computedStatus.getStatusChange() == null || this.computedStatus.isOutdated())
                && !jobState.isManualTrigger()) {
            trigger(commandConsumer, null, needWaitSchedule);
        }
    }

    private boolean isStageAcquired(StageRef stage) {
        return stageGroupState.isStageAcquiredBy(flowLaunch.getFlowLaunchId(), stage);
    }

    private void trigger(
            PendingCommandConsumer commandConsumer,
            @Nullable String triggeredBy,
            boolean needWaitSchedule
    ) {
        boolean needAcquireStage = jobState.getStage() != null && !isStageAcquired(jobState.getStage());

        StatusChange statusChange = needAcquireStage
                ? StatusChange.waitingForStage()
                : (needWaitSchedule ? StatusChange.waitngForSchedule() : StatusChange.queued());

        if (
                (!jobState.isWaitingForScheduleChangeType() && !jobState.isQueuedChangeType()) ||
                        (this.executorObject == null)
        ) {
            List<StatusChange> statusChangeList = new ArrayList<>();
            statusChangeList.add(statusChange);

            JobLaunch newLaunch = new JobLaunch(
                    jobState.getNextLaunchNumber(),
                    triggeredBy,
                    upstreams.stream()
                            .map(UpstreamLink::getEntity)
                            .filter(x -> x.getLastLaunch() != null)
                            .map(u -> new UpstreamLaunch(u.getId(), u.getLastLaunch().getNumber()))
                            .collect(Collectors.toList()),
                    statusChangeList
            );

            if (this.executorObject == null) {
                newLaunch.recordStatusChange(StatusChange.failed());
                newLaunch.setExecutionExceptionStacktrace(
                        String.format("Class not found %s", this.jobState.getExecutorContext())
                );
                newLaunch.setSandboxTaskStatus(null);
                newLaunch.setTaskletServerError(null);

                jobState.addLaunch(newLaunch);
                this.setComputedStatus(JobStatus.actual(statusChange, true));

                return;
            }

            newLaunch.setConsumedResources(collectResources());

            jobState.addLaunch(newLaunch);
        } else {
            jobState.getLastLaunch().recordStatusChange(statusChange);

            if (!Objects.equals(jobState.getLastLaunch().getTriggeredBy(), triggeredBy)) {
                jobState.getLastLaunch().setTriggeredBy(triggeredBy);
            }
        }

        this.setComputedStatus(JobStatus.actual(statusChange, !needAcquireStage && needWaitSchedule));

        if (needAcquireStage) {
            commandConsumer.unlockAndLockStage(
                    stageGroupState.getAcquiredStages(flowLaunch.getFlowLaunchId())
                            .stream()
                            .filter(flowLaunch::hasNoRunningJobsOnStage)
                            .collect(Collectors.toList()),
                    jobState.getStage(),
                    flowLaunch,
                    flowLaunch.isSkipStagesAllowed()
            );
        } else if (needWaitSchedule) {
            commandConsumer.waitingForScheduleJob(jobState, flowLaunch.getFlowLaunchId().asString());
        } else {
            commandConsumer.scheduleJob(jobState, flowLaunch.getFlowLaunchId().asString());
        }
    }


    ResourceRefContainer collectResources() {
        return resourceProvider.getResources(this, flowLaunch);
    }

    private void maybeTriggerIfWaitingForStage(PendingCommandConsumer commandConsumer) {
        JobLaunch jobLaunch = jobState.getLastLaunch();
        if (jobLaunch != null && jobLaunch.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE) {
            Preconditions.checkState(
                    jobState.getStage() != null, "Job cannot be in WAITING_FOR_STAGE state and has no stage"
            );

            if (isStageAcquired(jobState.getStage())) {
                if (jobState.isEnableJobSchedulerConstraint()
                        && jobState.getJobSchedulerConstraint() != null
                        && jobLaunch.getTriggeredBy() == null) {
                    waitingForScheduleJob(commandConsumer);
                } else {
                    scheduleJob(commandConsumer);
                }
            }
        }
    }

    private void maybeTriggerIfWaitingForSchedule(PendingCommandConsumer commandConsumer) {
        if (jobState.isWaitingForScheduleChangeType()) {
            scheduleJob(commandConsumer);
        }
    }

    private void scheduleJob(PendingCommandConsumer commandConsumer) {
        StatusChange statusChange = StatusChange.queued();
        jobState.getLastLaunch().recordStatusChange(statusChange);
        this.setComputedStatus(JobStatus.actual(statusChange, false));
        commandConsumer.scheduleJob(jobState, flowLaunch.getFlowLaunchId().asString());
    }

    private void waitingForScheduleJob(PendingCommandConsumer commandConsumer) {
        StatusChange statusChange = StatusChange.waitngForSchedule();
        jobState.getLastLaunch().recordStatusChange(statusChange);
        this.setComputedStatus(JobStatus.actual(statusChange, false));
        commandConsumer.waitingForScheduleJob(jobState, flowLaunch.getFlowLaunchId().asString());
    }

    private List<JobStatus> getUpstreamStatuses() {
        return upstreams.stream()
                .map(UpstreamLink::getEntity)
                .map(JobRuntime::getStatus)
                .collect(Collectors.toList());
    }

    private boolean isUpToDate(JobLaunch launch) {
        var upstreamMap = upstreams.stream()
                .map(UpstreamLink::getEntity)
                .collect(Collectors.toMap(JobRuntime::getId, Function.identity()));

        return switch (jobState.getCanRunWhen()) {
            case ANY_COMPLETED, ANY_FAILED -> launch.getUpstreamLaunches().stream()
                    .anyMatch(upstreamLaunch -> isUpstreamUpToDate(upstreamMap, upstreamLaunch));
            case ALL_COMPLETED -> launch.getUpstreamLaunches().stream()
                    .allMatch(upstreamLaunch -> isUpstreamUpToDate(upstreamMap, upstreamLaunch));
        };
    }

    private boolean isUpstreamUpToDate(Map<String, JobRuntime> upstreamMap,
                                       UpstreamLaunch upstreamLaunch) {
        String upstreamJobId = upstreamLaunch.getUpstreamJobId();
        JobRuntime upstream = upstreamMap.get(upstreamJobId);
        if (upstream == null) {
            return false;
        }
        JobLaunch upstreamLastLaunch = upstream.getLastLaunch();
        return !upstream.getJobState().isOutdated() &&
                upstreamLastLaunch != null &&
                upstreamLaunch.getLaunchNumber() == upstreamLastLaunch.getNumber();
    }

    private void setComputedStatus(JobStatus status) {
        this.computedStatus = status;
        this.jobState.setOutdated(this.computedStatus.isOutdated());
        this.jobState.setReadyToRun(this.computedStatus.isReadyToRun());
    }

    @Nullable
    public JobExecutorObject getExecutorObject() {
        return executorObject;
    }
}
