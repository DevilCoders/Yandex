package ru.yandex.ci.engine.launch;

import java.time.Duration;
import java.util.Objects;
import java.util.function.BiFunction;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.a.model.CleanupConditionConfig;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;
import ru.yandex.misc.bender.annotation.BenderDefaultValue;

@Slf4j
public class LaunchCleanupTask extends AbstractOnetimeTask<LaunchCleanupTask.Params> {

    private CiDb db;
    private BazingaTaskManager bazingaTaskManager;
    private FlowLaunchService flowLaunchService;
    private LaunchService launchService;
    private Duration rescheduleDelay;
    private FlowLaunchMutexManager flowLaunchMutexManager;

    public LaunchCleanupTask(@Nonnull CiDb db,
                             @Nonnull BazingaTaskManager bazingaTaskManager,
                             @Nonnull FlowLaunchService flowLaunchService,
                             @Nonnull LaunchService launchService,
                             @Nonnull Duration rescheduleDelay,
                             @Nonnull FlowLaunchMutexManager flowLaunchMutexManager) {
        super(LaunchCleanupTask.Params.class);
        this.db = db;
        this.bazingaTaskManager = bazingaTaskManager;
        this.flowLaunchService = flowLaunchService;
        this.launchService = launchService;
        this.rescheduleDelay = rescheduleDelay;
        this.flowLaunchMutexManager = flowLaunchMutexManager;
    }

    public LaunchCleanupTask(@Nonnull Launch.Id launchId, @Nullable CleanupReason cleanupReason) {
        super(new LaunchCleanupTask.Params(launchId, cleanupReason));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws InterruptedException {
        var launchId = params.getLaunchId();
        var cleanupReason = Objects.requireNonNullElse(params.getCleanupReason(), CleanupReason.FINISH);

        long started = System.currentTimeMillis();

        var maxDuration = getTimeout()
                .minus(rescheduleDelay)
                .minus(rescheduleDelay)
                .toMillis();

        State lastResult = null;
        while (true) {
            log.info("Cleaning up Launch {}, reason is {}", launchId, cleanupReason);

            BiFunction<Launch.Id, CleanupReason, State> action = lastResult == State.CLEANUP_FLOW
                    ? this::cleanupFlowWhenReady
                    : this::executeImpl;

            lastResult = flowLaunchMutexManager.acquireAndRun(
                    LaunchId.fromKey(launchId),
                    () -> db.currentOrTx(() -> action.apply(launchId, cleanupReason))
            );

            if (lastResult == State.BREAK) {
                return; // ---
            }

            var currentDuration = System.currentTimeMillis() - started;
            if (currentDuration >= maxDuration) {
                log.info("Task is running too long, rescheduling after {} millis out of {}",
                        currentDuration, maxDuration);
                bazingaTaskManager.schedule(new LaunchCleanupTask(launchId, cleanupReason));
                return;
            }
            log.info("Continue waiting for cleanup, {} millis out of {}...",
                    currentDuration, maxDuration);

            //noinspection BusyWait
            Thread.sleep(rescheduleDelay.toMillis());
        }
    }

    private State executeImpl(Launch.Id launchId, CleanupReason cleanupReason) {
        var launch = db.launches().get(launchId);

        var status = launch.getStatus();
        if (status.isTerminal()) {
            log.info("Launch in terminal state, will not cleanup");
            return State.BREAK; // ---
        }

        if (status == LaunchState.Status.CLEANING) {
            log.info("Launch is cleaning already, will not cleanup again");
            return State.BREAK; // ---
        }

        if (status == LaunchState.Status.DELAYED || status == LaunchState.Status.POSTPONE) {
            db.launches().save(launch.toBuilder()
                    .status(LaunchState.Status.CANCELED)
                    .build());
            log.warn("Launch {} in delayed state {}, will cancel it", launchId, status);
            return State.BREAK; // ---
        }

        @Nullable
        var flowLaunch = launch.getFlowLaunchId() != null ?
                db.flowLaunch().get(FlowLaunchId.of(launch.getFlowLaunchId())) :
                null;

        var cleanupCondition = getCleanupCondition(cleanupReason, launch, flowLaunch);
        log.info("Current cleanup action: {}", cleanupCondition);

        if (!cleanupCondition.isCleanup() && !cleanupCondition.isInterrupt()) {
            log.info("Launch is not scheduled for cleanup or interruption, skip");
            return State.BREAK; // ---
        }

        // Waiting for cleanup is a normal termination state, set upon all normal jobs completion
        if (!launch.isWaitingForCleanup()) {
            if (flowLaunch != null && !flowLaunch.hasCleanupJobs()) {
                var flowLaunchId = flowLaunch.getFlowLaunchId();
                log.info("Flow {} has no cleanup jobs, don't wait for completion", flowLaunchId);

                if (cleanupCondition.isInterrupt()) {
                    // Simply cancel the flow, because there are no cleanup jobs to execute
                    log.info("Cancel flow {} immediately", flowLaunchId);
                    launchService.cancel(
                            LaunchId.fromKey(launchId), UserUtils.loginForInternalCiProcesses(), cleanupReason.display()
                    );
                }
                return State.BREAK; // ---
            }

            // Now we have some cleanup jobs, need to check if it should be interrupted
            if (cleanupCondition.isInterrupt()) {
                log.info("Launch will be interrupted even if not waiting for cleanup");
            } else {
                Preconditions.checkState(cleanupCondition.isCleanup(), "Internal error. Must be isCleanup=true");
                // Need to reschedule operation, maybe cleanup later
                log.info("Launch is not waiting for cleanup yet, reschedule");
                return State.CONTINUE; // ---
            }
        }

        if (flowLaunch != null) {
            var flowLaunchId = flowLaunch.getFlowLaunchId();
            if (cleanupCondition.isCleanup()) {
                if (cleanupCondition.isInterrupt()) {
                    log.info("Flow {} is interrupting", flowLaunchId);
                    flowLaunchService.cancelJobs(flowLaunchId, JobType.DEFAULT);
                    return State.CLEANUP_FLOW; // Now wait for IDLE state in order to start cleanup
                }
                log.info("Flow {} is cleaning up", flowLaunchId);
                flowLaunchService.cleanupFlow(flowLaunchId);
            } else {
                Preconditions.checkState(cleanupCondition.isInterrupt(), "Internal error. Must be isInterrupt=true");
                log.info("Flow {} is canceling", flowLaunchId);
                launchService.cancel(
                        LaunchId.fromKey(launchId), UserUtils.loginForInternalCiProcesses(), cleanupReason.display()
                );
            }
        } else {
            db.launches().save(launch.toBuilder()
                    .status(LaunchState.Status.CANCELED)
                    .build());
            log.warn("No FlowLaunchId found in launch {}, skip cleanup, mark as canceled", launchId);
            return State.BREAK; // ---
        }

        log.info("Cleanup complete {}", launchId);
        return State.BREAK;
    }

    private State cleanupFlowWhenReady(Launch.Id launchId, CleanupReason cleanupReason) {
        var launch = db.launches().get(launchId);

        var status = launch.getStatus();
        if (status.isTerminal()) {
            log.info("Launch in terminal state, will not cleanup");
            return State.BREAK; // ---
        }

        var flowLaunchId = launch.getFlowLaunchId();
        Preconditions.checkState(flowLaunchId != null);

        if (status.isProcessing()) {
            log.info("Flow {} is still waiting for cleanup on status {}", flowLaunchId, status);
            return State.CLEANUP_FLOW;
        } else {
            log.info("Flow {} is cleaning up", flowLaunchId);
            flowLaunchService.cleanupFlow(FlowLaunchId.of(flowLaunchId));
            return State.BREAK;
        }
    }

    private CleanupConditionConfig getCleanupCondition(
            CleanupReason cleanupReason,
            Launch launch,
            @Nullable FlowLaunchEntity flowLaunch
    ) {
        var cleanupConfig = launch.getFlowInfo().getCleanupConfig();
        if (cleanupConfig != null) {
            for (var configReason : cleanupConfig.getConditions()) {
                if (configReason.getReasons().contains(cleanupReason)) {
                    return configReason;
                }
            }
            // Default behavior
            return CleanupConditionConfig.ofCleanup(cleanupReason);
        }

        boolean interruptByDefault =
                switch (cleanupReason) {
                    case NEW_DIFF_SET, PR_DISCARDED, PR_MERGED -> true;
                    case FINISH -> false;
                };

        // Do not interrupt by default when cleanup jobs are configured
        if (interruptByDefault && flowLaunch != null && !flowLaunch.hasCleanupJobs()) {
            return CleanupConditionConfig.ofInterrupt(cleanupReason);
        } else {
            return CleanupConditionConfig.ofCleanup(cleanupReason);
        }
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(15);
    }

    private enum State {
        CONTINUE,
        BREAK,
        CLEANUP_FLOW
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        String ciProcessId;
        int number;
        @BenderDefaultValue("FINISH")
        @Nullable
        CleanupReason cleanupReason;

        public Params(Launch.Id launchId, @Nullable CleanupReason cleanupReason) {
            this.ciProcessId = launchId.getProcessId();
            this.number = launchId.getLaunchNumber();
            this.cleanupReason = cleanupReason;
        }

        public Launch.Id getLaunchId() {
            return Launch.Id.of(ciProcessId, number);
        }
    }
}

