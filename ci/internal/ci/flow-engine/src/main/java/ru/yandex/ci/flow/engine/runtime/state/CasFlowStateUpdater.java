package ru.yandex.ci.flow.engine.runtime.state;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.events.CleanupFlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.DisableFlowGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobsGracefullyEvent;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.FlowCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.InterruptJobCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.PendingCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.ScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.WaitingForScheduleCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchStatistics;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;

/**
 * Работа с flow без stage-ей. Применяется для всех flow, кроме релизов.
 */
@Slf4j
@RequiredArgsConstructor
public class CasFlowStateUpdater implements FlowStateUpdater {
    @Nonnull
    private final FlowStateRevisionService stateRevisionService;
    @Nonnull
    private final FlowStateCalculator stateCalculator;
    @Nonnull
    private final JobScheduler jobScheduler;
    @Nonnull
    private final JobWaitingScheduler jobWaitingSchedulerImpl;
    @Nonnull
    private final JobInterruptionService jobInterruptionService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final FlowLaunchUpdateDelegate flowLaunchUpdateDelegate;
    @Nonnull
    private final FlowLaunchMutexManager flowLaunchMutexManager;

    @Override
    public FlowLaunchEntity activateLaunch(FlowLaunchEntity flowLaunch) {
        var recalcResult = stateCalculator.recalc(flowLaunch, null);

        var updatedFlowLaunch = db.currentOrTx(() -> {
            var updated = db.flowLaunch().save(recalcResult.getFlowLaunch());
            flowLaunchUpdateDelegate.flowLaunchUpdated(updated);
            processCommands(flowLaunch.getProjectId(), flowLaunch.getLaunchId(), recalcResult.getPendingCommands());
            return updated;
        });
        stateRevisionService.setIfLessThan(updatedFlowLaunch.getIdString(), 0);
        return updatedFlowLaunch;
    }

    @Override
    public FlowLaunchEntity recalc(FlowLaunchId flowLaunchId, LaunchId launchId, FlowEvent event) {
        return recalcAndUpdateVersion(flowLaunchId, launchId, event);
    }

    @Override
    public void disableLaunchGracefully(
            FlowLaunchId flowLaunchId, LaunchId launchId, boolean ignoreUninterruptableStage
    ) {
        recalcAndUpdateVersion(flowLaunchId, launchId, new DisableFlowGracefullyEvent(ignoreUninterruptableStage));
    }

    @Override
    public void disableJobsInLaunchGracefully(
            FlowLaunchId flowLaunchId,
            LaunchId launchId,
            List<String> jobIds,
            boolean ignoreUninterruptableStage,
            boolean killJobs
    ) {
        recalcAndUpdateVersion(flowLaunchId, launchId, new DisableJobsGracefullyEvent(false, killJobs, jobIds));
    }

    @Override
    public void cleanupFlowLaunch(FlowLaunchId flowLaunchId, LaunchId launchId) {
        recalcAndUpdateVersion(flowLaunchId, launchId, new CleanupFlowEvent());
    }

    private FlowLaunchEntity recalcAndUpdateVersion(FlowLaunchId flowLaunchId, LaunchId launchId, FlowEvent event) {
        FlowLaunchEntity flowLaunch = recalcInTransaction(flowLaunchId, launchId, event);

        try {
            stateRevisionService.setIfLessThan(flowLaunch.getIdString(), flowLaunch.getStateVersion());
        } catch (RuntimeException e) {
            log.warn("Unable to update revision in zookeeper for flow launch " + flowLaunch.getIdString(), e);
        }

        return flowLaunch;
    }

    private FlowLaunchEntity recalcInTransaction(FlowLaunchId flowLaunchId, LaunchId launchId, FlowEvent event) {
        return flowLaunchMutexManager.acquireAndRun(
                launchId,
                () -> db.currentOrTx(() -> {
                    var flowLaunch = db.flowLaunch().get(flowLaunchId);
                    flowLaunch = singleFlowRecalcAndSave(flowLaunch, event);

                    flowLaunch = db.flowLaunch().save(flowLaunch);
                    flowLaunchUpdateDelegate.flowLaunchUpdated(flowLaunch);
                    return flowLaunch;
                })
        );
    }

    private FlowLaunchEntity singleFlowRecalcAndSave(FlowLaunchEntity flowLaunch, FlowEvent event) {
        AtomicReference<List<PendingCommand>> commands = new AtomicReference<>();

        var recalcResult = stateCalculator.recalc(flowLaunch, event);
        commands.set(recalcResult.getPendingCommands());
        flowLaunch = recalcResult.getFlowLaunch();

        var statistics = LaunchStatistics.fromLaunch(flowLaunch, Collections.emptySet(), Collections.emptySet());

        flowLaunch = flowLaunch.toBuilder()
                .statistics(statistics)
                .state(LaunchState.fromStatistics(statistics))
                .build();

        log.info(
                String.format(
                        "Launch %s, event: %s, state: %s, statistics: %s",
                        flowLaunch.getIdString(), event, flowLaunch.getState(), flowLaunch.getStatistics()
                )
        );

        List<FlowEvent> recalcEvents = processCommands(
                flowLaunch.getProjectId(), flowLaunch.getLaunchId(), commands.get()
        );
        for (FlowEvent recalcEvent : recalcEvents) {
            flowLaunch = singleFlowRecalcAndSave(flowLaunch, recalcEvent);
        }

        return flowLaunch;
    }

    private List<FlowEvent> processCommands(String projectId, LaunchId launchId, List<PendingCommand> pendingCommands) {
        List<FlowEvent> recalcEvents = new ArrayList<>();
        for (PendingCommand command : pendingCommands) {
            log.info("Processing commands: {}", command);
            if (command instanceof ScheduleCommand) {
                ScheduleCommand sc = (ScheduleCommand) command;
                jobScheduler.scheduleFlow(projectId, launchId, sc.getJobLaunchId());
            } else if (command instanceof InterruptJobCommand) {
                jobInterruptionService.notifyExecutor((InterruptJobCommand) command);
            } else if (command instanceof WaitingForScheduleCommand) {
                jobWaitingSchedulerImpl.schedule((WaitingForScheduleCommand) command);
            } else if (command instanceof FlowCommand) {
                recalcEvents.add(((FlowCommand) command).getEvent());
            } else {
                throw new IllegalStateException("Unknown command type: " + command.toString());
            }
        }
        return recalcEvents;
    }
}
