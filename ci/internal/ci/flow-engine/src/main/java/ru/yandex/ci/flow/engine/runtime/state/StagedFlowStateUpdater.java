package ru.yandex.ci.flow.engine.runtime.state;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchDisplacedBy;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;
import ru.yandex.ci.flow.engine.runtime.LaunchAutoReleaseDelegate;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.events.DisableFlowGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobsGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.FlowCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.InterruptJobCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.LaunchAutoReleaseCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.PendingCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.RecalcCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.ScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.StageGroupCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.WaitingForScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchStatistics;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;
import ru.yandex.ci.util.jackson.SerializationUtils;
import ru.yandex.lang.NonNullApi;

/**
 * Умеет атомарно обновить несколько флоу и документ со стейджами (секциями).
 * Применяется только для релизных flow.
 */
@Slf4j
@RequiredArgsConstructor
public class StagedFlowStateUpdater implements FlowStateUpdater {
    @Nonnull
    private final FlowStateCalculator flowStateCalculator;
    @Nonnull
    private final JobScheduler jobScheduler;
    @Nonnull
    private final JobWaitingScheduler jobWaitingSchedulerImpl;
    @Nonnull
    private final FlowStateRevisionService stateRevisionService;
    @Nonnull
    private final JobInterruptionService jobInterruptionService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final FlowLaunchUpdateDelegate flowLaunchUpdateDelegate;
    @Nonnull
    private final LaunchAutoReleaseDelegate launchAutoReleaseDelegate;
    @Nonnull
    private final FlowLaunchMutexManager flowLaunchMutexManager;

    @Override
    public FlowLaunchEntity activateLaunch(FlowLaunchEntity flowLaunch) {
        return flowLaunchMutexManager.acquireAndRun(
                flowLaunch.getLaunchId(),
                () -> db.currentOrTx(() -> {
                    var flowLaunchCopy = SerializationUtils.copy(flowLaunch, FlowLaunchEntity.class);
                    db.stageGroup().initStage(flowLaunch.getStageGroupId().orElseThrow());
                    return recalc(flowLaunchCopy, null);
                })
        );
    }

    @Override
    public FlowLaunchEntity recalc(FlowLaunchId flowLaunchId, LaunchId launchId, @Nullable FlowEvent event) {
        return flowLaunchMutexManager.acquireAndRun(
                launchId,
                () -> db.currentOrTx(() -> {
                    var flowLaunch = db.flowLaunch().get(flowLaunchId);
                    return recalc(flowLaunch, event);
                })
        );
    }

    private FlowLaunchEntity recalc(FlowLaunchEntity launch, @Nullable FlowEvent event) {
        var stageGroupId = launch.getStageGroupId().orElseThrow();
        var flowLaunchId = launch.getFlowLaunchId();

        CommandContainer commandContainer = new CommandContainer(new FlowCommand(flowLaunchId, event));

        StageGroupState stageGroupState = db.stageGroup().get(stageGroupId);
        log.info("Stage group state (in recalc()): {}", stageGroupState);
        CommandExecutor commandExecutor = new CommandExecutor(launch, stageGroupState, commandContainer);

        while (commandContainer.hasRecalcCommands()) {
            PendingCommand command = commandContainer.nextRecalcCommand();
            commandExecutor.executeRecalcCommand(command);
        }

        log.info("Stages in command executor after recalc commands (in recalc()): {}",
                commandExecutor.stageGroupState);

        FlowLaunchEntity updatingLaunch = commandExecutor.flowLaunches.get(flowLaunchId);
        var statistics = LaunchStatistics.fromLaunch(updatingLaunch, stageGroupState);

        updatingLaunch = updatingLaunch.toBuilder()
                .statistics(statistics)
                .state(LaunchState.fromStatistics(statistics))
                .build();

        log.info(
                String.format(
                        "Launch %s, event: %s, state: %s, statistics: %s",
                        updatingLaunch.getIdString(), event, updatingLaunch.getState(), updatingLaunch.getStatistics()
                )
        );

        commandExecutor.flowLaunches.put(updatingLaunch.getFlowLaunchId(), updatingLaunch);
        commandExecutor.saveEntities();

        Runnable rescheduleFlowConsumer = () -> {
            log.info("About to reschedule flow check {}", flowLaunchId);
            jobScheduler.scheduleStageRecalc(launch.getProjectId(), flowLaunchId);
        };

        // Try displacement
        var updatingLaunchRef = updatingLaunch;
        new DisplacementStagesUpdater().tryDisplacement(
                flowLaunchId,
                stageGroupState,
                commandExecutor::getFlowLaunch,
                cancelLaunch -> {
                    log.info("About to cancel flow {}...", cancelLaunch.getFlowLaunchId());
                    tryCancelDisplaced(cancelLaunch, updatingLaunchRef, rescheduleFlowConsumer);
                },
                rescheduleFlowConsumer);

        return updatingLaunch;
    }

    @Override
    public void disableLaunchGracefully(FlowLaunchId flowLaunchId, LaunchId launchId,
                                        boolean ignoreUninterruptableStage) {
        recalc(flowLaunchId, launchId, new DisableFlowGracefullyEvent(ignoreUninterruptableStage));
    }

    @Override
    public void disableJobsInLaunchGracefully(
            FlowLaunchId flowLaunchId,
            LaunchId launchId,
            List<String> jobIds,
            boolean ignoreUninterruptableStage,
            boolean killJobs
    ) {
        recalc(flowLaunchId, launchId, new DisableJobsGracefullyEvent(ignoreUninterruptableStage, killJobs, jobIds));
    }

    @Override
    public void cleanupFlowLaunch(FlowLaunchId flowLaunchId, LaunchId launchId) {
        throw new IllegalStateException("Stages does not support cleanup operations: " + flowLaunchId);
    }

    private void tryCancelDisplaced(FlowLaunchEntity cancelingEntity,
                                    FlowLaunchEntity currentEntity,
                                    Runnable rescheduleFlowConsumer) {
        var cancelingFlowLaunchId = cancelingEntity.getFlowLaunchId();
        var cancelLaunch = db.launches().get(cancelingEntity.getLaunchId());
        if (!cancelLaunch.getStatus().isTerminal()) {

            // Displacement options could be changed for each flow, has to check it explicitly
            var changes = cancelLaunch.getDisplacementChanges();
            if (!changes.isEmpty()) {
                var lastChange = changes.get(changes.size() - 1);
                log.info("Last Displacement Change: {}", lastChange);
                if (lastChange.getState() == Common.DisplacementState.DS_DENY) {
                    log.info("Displacement is denied, rescheduling...");
                    rescheduleFlowConsumer.run();
                    return; // ---
                }
            }

            // Must be marked as 'CANCELING', it will end with status 'SUCCESS' otherwise
            var version = currentEntity.getLaunchInfo().getVersion();
            Launch cancellingLaunch = cancelLaunch.toBuilder()
                    .status(ru.yandex.ci.core.launch.LaunchState.Status.CANCELLING)
                    .cancelledReason("Displaced automatically by #" + version.asString())
                    .displaced(true)
                    .displacedBy(LaunchDisplacedBy.of(currentEntity.getLaunchId().toKey(), version))
                    .build();
            db.launches().save(cancellingLaunch);
            disableJobsInLaunchGracefully(
                    cancelingFlowLaunchId,
                    cancelingEntity.getLaunchId(),
                    List.copyOf(cancelingEntity.getJobs().keySet()),
                    false,
                    false
            );
            disableLaunchGracefully(cancelingFlowLaunchId, cancelingEntity.getLaunchId(), false);
        } else {
            log.error("Internal error, cannot displace flow {}, it's already in terminal state: {}",
                    cancelingFlowLaunchId, cancelLaunch.getStatus());
        }
    }

    @NonNullApi
    class CommandExecutor {
        final FlowLaunchEntity primaryLaunch;
        final Map<FlowLaunchId, FlowLaunchEntity> flowLaunches = new LinkedHashMap<>();
        final StageGroupState stageGroupState;
        final CommandContainer commandContainer;

        CommandExecutor(FlowLaunchEntity flowLaunch, StageGroupState stageGroupState,
                        CommandContainer commandContainer) {
            this.primaryLaunch = flowLaunch;
            this.flowLaunches.put(flowLaunch.getFlowLaunchId(), flowLaunch);
            this.stageGroupState = stageGroupState;
            this.commandContainer = commandContainer;
        }

        void executeRecalcCommand(PendingCommand command) {
            if (command instanceof FlowCommand flowCommand) {
                FlowLaunchEntity flowLaunch = getFlowLaunch(flowCommand.getFlowLaunchId());
                var recalcResult = flowStateCalculator.recalc(
                        flowLaunch, flowCommand.getEvent(), stageGroupState
                );
                commandContainer.addAll(recalcResult.getPendingCommands());
                flowLaunches.put(flowLaunch.getFlowLaunchId(), recalcResult.getFlowLaunch());
            } else if (command instanceof StageGroupCommand stageGroupCommand) {
                log.info("Executing stage group command {}", command);
                commandContainer.addAll(stageGroupCommand.execute(stageGroupState));
            } else {
                throw new IllegalStateException("Unknown command type " + command);
            }
        }

        void saveEntities() {
            if (commandContainer.allRecalcCommands.stream().anyMatch(c -> c instanceof StageGroupCommand)) {
                db.stageGroup().save(stageGroupState);
            }

            for (FlowLaunchEntity flowLaunch : flowLaunches.values()) {
                var flowLaunchUpdated = db.flowLaunch().save(flowLaunch);
                flowLaunchUpdateDelegate.flowLaunchUpdated(flowLaunchUpdated);

                stateRevisionService.setIfLessThan(
                        flowLaunchUpdated.getIdString(), flowLaunchUpdated.getStateVersion());

                commandContainer.getNonRecalcCommands(flowLaunchUpdated.getFlowLaunchId(), ScheduleCommand.class)
                        .forEach(command -> {
                            jobScheduler.scheduleFlow(
                                    primaryLaunch.getProjectId(), primaryLaunch.getLaunchId(), command.getJobLaunchId()
                            );
                            log.info("Job {} is scheduled", command.getJobLaunchId());
                        });

                commandContainer.getNonRecalcCommands(
                        flowLaunchUpdated.getFlowLaunchId(), WaitingForScheduleCommand.class
                ).forEach(command -> {
                    jobWaitingSchedulerImpl.schedule(command.getJobLaunchId(), command.getDate());
                    log.info("Job {} is waiting for schedule", command.getJobLaunchId());
                });

                commandContainer.getNonRecalcCommands(
                        flowLaunchUpdated.getFlowLaunchId(), InterruptJobCommand.class
                ).forEach(jobInterruptionService::notifyExecutor);

                commandContainer.getNonRecalcCommands(
                        flowLaunchUpdated.getFlowLaunchId(), LaunchAutoReleaseCommand.class
                ).forEach(command -> launchAutoReleaseDelegate.scheduleLaunchAfterFlowUnlockedStage(
                        command.getFlowLaunch().getFlowLaunchId(),
                        flowLaunchUpdated.getLaunchId()
                ));
            }
        }

        FlowLaunchEntity getFlowLaunch(FlowLaunchId id) {
            return flowLaunches.computeIfAbsent(id, launchId -> db.flowLaunch().get(launchId));
        }
    }

    static class CommandContainer {
        private final ArrayDeque<RecalcCommand> remainingRecalcCommands = new ArrayDeque<>();
        private final Multimap<FlowLaunchId, PendingCommand> flowLaunchIdToCommands = HashMultimap.create();
        private final List<RecalcCommand> allRecalcCommands = new ArrayList<>();

        CommandContainer(PendingCommand... pendingCommands) {
            addAll(Arrays.asList(pendingCommands));
        }

        void addAll(List<? extends PendingCommand> commands) {
            for (PendingCommand command : commands) {
                if (command instanceof RecalcCommand recalcCommand) {
                    remainingRecalcCommands.add(recalcCommand);
                    allRecalcCommands.add(recalcCommand);
                } else if (command instanceof ScheduleCommand scheduleCommand) {
                    flowLaunchIdToCommands.put(scheduleCommand.getJobLaunchId().getFlowLaunchId(), scheduleCommand);
                } else if (command instanceof InterruptJobCommand interruptJobCommand) {
                    flowLaunchIdToCommands.put(
                            interruptJobCommand.getJobLaunchId().getFlowLaunchId(),
                            interruptJobCommand
                    );
                } else if (command instanceof WaitingForScheduleCommand waitingForScheduleCommand) {
                    flowLaunchIdToCommands.put(
                            waitingForScheduleCommand.getJobLaunchId().getFlowLaunchId(),
                            waitingForScheduleCommand
                    );
                } else if (command instanceof LaunchAutoReleaseCommand launchAutoReleaseCommand) {
                    FlowLaunchEntity flowLaunch = launchAutoReleaseCommand.getFlowLaunch();
                    flowLaunchIdToCommands.put(flowLaunch.getFlowLaunchId(), command);
                } else {
                    throw new IllegalStateException("Unknown command type: " + command);
                }

            }
        }

        boolean hasRecalcCommands() {
            return !remainingRecalcCommands.isEmpty();
        }

        PendingCommand nextRecalcCommand() {
            return remainingRecalcCommands.pop();
        }

        <T> List<T> getNonRecalcCommands(FlowLaunchId flowLaunchId, Class<T> clazz) {
            return flowLaunchIdToCommands.get(flowLaunchId).stream()
                    .filter(clazz::isInstance)
                    .map(clazz::cast)
                    .collect(Collectors.toList());
        }
    }
}
