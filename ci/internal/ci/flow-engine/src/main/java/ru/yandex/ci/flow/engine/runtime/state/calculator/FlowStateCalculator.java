package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.time.Clock;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.CancelGracefulDisablingEvent;
import ru.yandex.ci.flow.engine.runtime.events.CleanupFlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.DisableFlowGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobsGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptingEvent;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.GenerateJobsEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobKilledEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobLaunchEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.UnlockStageEvent;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.PendingCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.GracefulDisablingState;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

/**
 * Осуществляет пересчёт состояния флоу в зависимости от внешнего события.
 */
@Slf4j
@RequiredArgsConstructor
public class FlowStateCalculator {

    @Nonnull
    private final SourceCodeService sourceCodeService;
    @Nonnull
    private final ResourceProvider resourceProvider;
    @Nonnull
    private final UpstreamResourcesCollector upstreamResourcesCollector;
    @Nonnull
    private final JobsMultiplyCalculator jobsTemplate;
    @Nonnull
    private final Clock clock;
    @Nonnull
    private final FlowLaunchMutexManager flowLaunchMutexManager;

    public RecalcResult recalc(FlowLaunchEntity flowLaunch, @Nullable FlowEvent event) {
        return flowLaunchMutexManager.acquireAndRun(
                flowLaunch.getLaunchId(),
                () -> doRecalc(flowLaunch, event, null)
        );
    }

    public RecalcResult recalc(FlowLaunchEntity flowLaunch,
                               @Nullable FlowEvent event,
                               @Nullable StageGroupState stageGroupState) {
        return flowLaunchMutexManager.acquireAndRun(
                flowLaunch.getLaunchId(),
                () -> doRecalc(flowLaunch, event, stageGroupState)
        );
    }

    private RecalcResult doRecalc(FlowLaunchEntity flowLaunch,
                                  @Nullable FlowEvent event,
                                  @Nullable StageGroupState stageGroupState) {
        log.info("Recalculating flow launch {} because of event {}", flowLaunch.getIdString(), event);

        PendingCommandConsumer commandConsumer = new PendingCommandConsumer(clock);

        if (event instanceof CleanupFlowEvent) {
            flowLaunch = registerCleanupJobs(flowLaunch);
        }

        if (event instanceof DisableFlowGracefullyEvent disableFlowGracefullyEvent) {
            flowLaunch = flowLaunch.withGracefulDisablingState(
                    GracefulDisablingState.of(true, disableFlowGracefullyEvent.shouldIgnoreUninterruptableStage())
            );

            if (canDisableFlowLaunch(flowLaunch, stageGroupState)) {
                flowLaunch = disableFlowLaunch(flowLaunch, commandConsumer);
            }
        }

        if (event instanceof DisableJobsGracefullyEvent disableJobsGracefullyEvent) {
            disableJobsGracefully(
                    flowLaunch,
                    commandConsumer,
                    disableJobsGracefullyEvent,
                    stageGroupState
            );
        }

        if (event instanceof UnlockStageEvent unlockStageEvent) {
            maybeUnlockStage(commandConsumer, stageGroupState, flowLaunch, unlockStageEvent.getStageRef());
        }

        if (event instanceof CancelGracefulDisablingEvent) {
            Preconditions.checkState(
                    !flowLaunch.isDisabled(),
                    "Flow launch %s is already disabled, unable to cancel disabling", flowLaunch.getIdString()
            );

            flowLaunch = flowLaunch.withGracefulDisablingState(null);
        }

        if (event instanceof GenerateJobsEvent generateJobsEvent) {
            flowLaunch = jobsTemplate.multiplyJobs(
                    generateJobsEvent.getJobContextFactory(),
                    flowLaunch,
                    generateJobsEvent.getJobId());
        }

        recalcJobs(event, buildFlowLaunchRuntime(flowLaunch, stageGroupState), commandConsumer);

        maybeUnlockPastStages(flowLaunch, event, stageGroupState, commandConsumer);

        if (flowLaunch.isStaged()) {
            flowLaunch = maybeFinishFlow(buildFlowLaunchRuntime(flowLaunch, stageGroupState), commandConsumer);
        }

        if (flowLaunch.isDisablingGracefully()) {
            if (canDisableFlowLaunch(flowLaunch, stageGroupState)) {
                flowLaunch = disableFlowLaunch(flowLaunch, commandConsumer);
            }
        }

        maybeDisableJobs(flowLaunch);

        return RecalcResult.of(flowLaunch, commandConsumer.getCommands());
    }

    FlowLaunchRuntime buildFlowLaunchRuntime(
            FlowLaunchEntity flowLaunch,
            @Nullable StageGroupState stageGroupState
    ) {
        return new FlowLaunchRuntime(
                flowLaunch,
                stageGroupState,
                sourceCodeService,
                resourceProvider,
                upstreamResourcesCollector
        );
    }

    private void maybeUnlockPastStages(
            FlowLaunchEntity flowLaunch,
            FlowEvent event,
            @Nullable StageGroupState stageGroupState,
            PendingCommandConsumer commandConsumer
    ) {

        if (event instanceof JobLaunchEvent jobLaunchEvent) {
            JobState jobState = flowLaunch.getJobState(jobLaunchEvent.getJobId());
            StatusChangeType lastStatusChangeType = jobState.getLastStatusChangeType();
            if (lastStatusChangeType.isFinished()) {
                maybeUnlockStage(commandConsumer, stageGroupState, flowLaunch, jobState);
            }

            if (event instanceof JobRunningEvent && stageGroupState != null) {
                stageGroupState.getAcquiredStages(flowLaunch.getFlowLaunchId())
                        .forEach(stage -> maybeUnlockStage(commandConsumer, stageGroupState, flowLaunch, stage));
            }
        }
    }

    //TODO unmodifiable jobs
    private void maybeDisableJobs(FlowLaunchEntity flowLaunch) {
        flowLaunch.getJobs().values()
                .stream()
                .filter(JobState::isDisabling)
                .filter(job -> !job.isInProgress()
                        || job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE
                        || job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_SCHEDULE)
                .forEach(job -> {
                    if (job.getStage() == null
                            || job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE
                            || job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_SCHEDULE) {
                        job.setDisabled(true);
                        return;
                    }

                    StoredStage stage = flowLaunch.getStage(job.getStage());
                    boolean canStageBeInterrupted = stage.isCanBeInterrupted();

                    if (canStageBeInterrupted || job.shouldIgnoreUninterruptibleStage()) {
                        job.setDisabled(true);
                        return;
                    }

                    if (flowLaunch.getJobsByStage(job.getStage()).stream().allMatch(JobState::isSuccessful)) {
                        job.setDisabled(true);
                    }
                });
    }

    public FlowLaunchEntity registerCleanupJobs(FlowLaunchEntity flowLaunch) {
        var cleanupJobs = flowLaunch.getCleanupJobs();
        if (cleanupJobs.isEmpty()) {
            log.error("Attempt to register cleanup jobs for {}, but no cleanup jobs exists...",
                    flowLaunch.getId());
            return flowLaunch;
        }

        if (flowLaunch.isCleanupRunning()) {
            log.error("Flow {} is already running cleanup, skip...", flowLaunch.getId());
            return flowLaunch;
        }

        log.info("Cleaning up flow {}, state is {}, using jobs {}",
                flowLaunch.getId(), flowLaunch.getState(), cleanupJobs.keySet());

        var builder = flowLaunch.toBuilder();

        var jobs = new LinkedHashMap<>(flowLaunch.getJobs());
        cleanupJobs.keySet().forEach(jobId ->
                Preconditions.checkState(!jobs.containsKey(jobId),
                        "Job with id %s already exists, cannot add cleanup job",
                        jobId));
        jobs.putAll(cleanupJobs);

        builder.jobs(jobs);
        return builder.build();
    }

    private void recalcJobs(@Nullable FlowEvent event,
                            FlowLaunchRuntime flowLaunchRuntime,
                            PendingCommandConsumer commandConsumer) {
        for (JobRuntime job : flowLaunchRuntime.getJobsWithoutDownstreams()) {
            job.processEvent(commandConsumer, event);
        }
    }

    private void disableJobsGracefully(
            FlowLaunchEntity flowLaunch,
            PendingCommandConsumer commandConsumer,
            DisableJobsGracefullyEvent event,
            @Nullable StageGroupState stageGroupState
    ) {
        Predicate<JobState> jobMatches = job -> event.getJobIds().contains(job.getJobId());
        Predicate<JobState> isJobInProgress = JobState::isInProgress;

        flowLaunch.getJobs().values().stream()
                .filter(jobMatches)
                .filter(isJobInProgress.negate())
                .forEach(job -> {
                    boolean canBeInterrupted = canJobBeInterrupted(flowLaunch, job, event, stageGroupState);

                    if (canBeInterrupted) {
                        log.info("Job {} is idle and can be interrupted, setting disabled to true", job.getJobId());
                        job.setDisabled(true);
                    } else {
                        log.info(
                                "Job {} is idle and can not be interrupted (due to uninterruptible stage), " +
                                        "setting disabling to true",
                                job.getJobId()
                        );

                        job.setDisabling(true, event.isIgnoreUninterruptableStage());
                    }
                });

        flowLaunch.getJobs().values().stream()
                .filter(jobMatches)
                .filter(isJobInProgress)
                .forEach(job -> {
                    job.setDisabling(true, event.isIgnoreUninterruptableStage());
                    boolean canBeInterrupted = canJobBeInterrupted(flowLaunch, job, event, stageGroupState);
                    FullJobLaunchId fullJobId = job.getLastLaunch() == null ? null :
                            new FullJobLaunchId(
                                    flowLaunch.getFlowLaunchId(),
                                    job.getJobId(),
                                    job.getLastLaunch().getNumber()
                            );

                    if (job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE
                            || job.getLastStatusChangeType() == StatusChangeType.WAITING_FOR_SCHEDULE) {
                        commandConsumer.killJob(
                                flowLaunch.getFlowLaunchId(),
                                new JobKilledEvent(job.getJobId(), job.getLastLaunch().getNumber())
                        );

                        job.setDisabled(true);
                    }

                    if (canBeInterrupted && job.getLastStatusChangeType() == StatusChangeType.RUNNING) {
                        log.info("Job {} is running and can be interrupted, interrupting", job.getJobId());

                        commandConsumer.executorInterruptingJob(
                                flowLaunch.getFlowLaunchId(),
                                new ExecutorInterruptingEvent(job.getJobId(), job.getLastLaunch().getNumber(), null)
                        );

                        commandConsumer.interruptJob(fullJobId);
                    } else if (event.isKillJobs() && job.getLastStatusChangeType() == StatusChangeType.RUNNING) {
                        log.info("Job {} is running and will be killed", job.getJobId());

                        commandConsumer.executorInterruptingJob(
                                flowLaunch.getFlowLaunchId(),
                                new ExecutorInterruptingEvent(job.getJobId(), job.getLastLaunch().getNumber(), null)
                        );

                        commandConsumer.killJob(fullJobId);
                    }
                });
    }

    private boolean canJobBeInterrupted(
            FlowLaunchEntity flowLaunch,
            JobState job,
            DisableJobsGracefullyEvent event,
            @Nullable StageGroupState stageGroupState
    ) {
        if (event.isIgnoreUninterruptableStage() || stageGroupState == null) {
            return true;
        }

        Set<? extends StageRef> acquiredStages = stageGroupState.getAcquiredStages(flowLaunch.getFlowLaunchId());

        if (job.getStage() != null && acquiredStages.contains(job.getStage())) {
            StoredStage stage = flowLaunch.getStage(job.getStage());
            return stage.isCanBeInterrupted();
        }

        return true;
    }

    private void maybeUnlockStage(PendingCommandConsumer commandConsumer, StageGroupState stageGroupState,
                                  FlowLaunchEntity flowLaunch, JobState jobState) {
        StageRef stage = jobState.getStage();
        if (stage == null || stageGroupState.isStageAcquiredBy(flowLaunch.getFlowLaunchId(), stage)) {
            maybeUnlockStage(commandConsumer, stageGroupState, flowLaunch, stage);
        } else {
            log.info("Unlocking stage {} skipped because flow doesn't hold the stage", stage.getId());
        }
    }

    private void maybeUnlockStage(PendingCommandConsumer commandConsumer, StageGroupState stageGroupState,
                                  FlowLaunchEntity flowLaunch, @Nullable StageRef stage) {
        if (stage != null) {
            Preconditions.checkState(stageGroupState != null);
            boolean hasGreaterStagesAcquired = hasGreaterStagesAcquired(stage, stageGroupState, flowLaunch);
            boolean hasNoRunningJobsOnStage = flowLaunch.hasNoRunningJobsOnStage(stage);

            if (hasGreaterStagesAcquired && hasNoRunningJobsOnStage) {
                log.info("Unlocking stage {} as it has no running jobs anymore", stage);
                commandConsumer.unlockStage(stage, flowLaunch);
            } else {
                log.info(
                        "Stage {} cannot be unlocked yet, hasGreaterStagesAcquired = {}, hasNoRunningJobsOnStage = {}",
                        stage, hasGreaterStagesAcquired, hasNoRunningJobsOnStage
                );
            }
        }
    }

    private boolean hasGreaterStagesAcquired(StageRef stage, StageGroupState stageGroupState,
                                             FlowLaunchEntity flowLaunch) {
        Set<? extends StageRef> acquiredStages = stageGroupState.getAcquiredStages(flowLaunch.getFlowLaunchId());
        List<String> acquiredStageIds = acquiredStages.stream().map(StageRef::getId).collect(Collectors.toList());
        return flowLaunch.isStageLessThanAny(stage.getId(), acquiredStageIds);
    }

    private boolean canDisableFlowLaunch(FlowLaunchEntity flowLaunch, @Nullable StageGroupState stageGroupState) {
        boolean hasNoRunningJobs = getRunningJobs(flowLaunch).isEmpty();
        if (stageGroupState == null) {
            return hasNoRunningJobs;
        }

        boolean acquiredStagesAreInterruptable = areAcquiredStagesInterruptable(flowLaunch, stageGroupState);

        return hasNoRunningJobs
                && (acquiredStagesAreInterruptable || flowLaunch.shouldIgnoreUninterruptibleStages());
    }

    private boolean areAcquiredStagesInterruptable(FlowLaunchEntity flowLaunch, StageGroupState stageGroupState) {
        return stageGroupState.getAcquiredStages(flowLaunch.getFlowLaunchId())
                .stream()
                .map(flowLaunch::getStage)
                .allMatch(StoredStage::isCanBeInterrupted);
    }

    private List<JobState> getRunningJobs(FlowLaunchEntity flowLaunch) {
        return flowLaunch.getJobs()
                .values()
                .stream()
                .filter(job -> job.isInProgress()
                        && !StatusChangeType.WAITING_FOR_STAGE.equals(job.getLastStatusChangeType())
                        && !StatusChangeType.WAITING_FOR_SCHEDULE.equals(job.getLastStatusChangeType()))
                .collect(Collectors.toList());
    }

    private FlowLaunchEntity maybeFinishFlow(FlowLaunchRuntime flowLaunchRuntime,
                                             PendingCommandConsumer commandConsumer) {
        List<JobRuntime> lastStageTerminationJobs = flowLaunchRuntime.getJobsWithoutDownstreams().stream()
                .filter(j -> j.getJobState().getStage() != null)
                .collect(Collectors.toList());

        FlowLaunchEntity flowLaunch = flowLaunchRuntime.getFlowLaunch();
        boolean noJobsRunning = getRunningJobs(flowLaunch).isEmpty();

        if (lastStageTerminationJobs.stream().allMatch(j -> j.getJobState().isSuccessful()) && noJobsRunning) {
            flowLaunch = disableFlowLaunch(flowLaunch, commandConsumer);
        }
        return flowLaunch;
    }

    private FlowLaunchEntity disableFlowLaunch(FlowLaunchEntity flowLaunch, PendingCommandConsumer commandConsumer) {
        flowLaunch = flowLaunch.toBuilder()
                .disabled(true)
                .gracefulDisablingState(null)
                .build();

        if (flowLaunch.isStaged()) {
            commandConsumer.removeFromStageQueue(flowLaunch);
        }
        return flowLaunch;
    }

    @Value(staticConstructor = "of")
    public static class RecalcResult {
        FlowLaunchEntity flowLaunch;
        List<PendingCommand> pendingCommands;
    }
}
