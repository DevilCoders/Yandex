package ru.yandex.ci.flow.engine.runtime.state;

import java.util.HashSet;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.stage.StageRefImpl;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchStatistics;

@Slf4j
@AllArgsConstructor
public class DisplacementStagesUpdater {

    /**
     * Try to make stage displacement (i.e. close previous flow if possible)
     *
     * @param flowLaunchId           current flow launch id
     * @param stageGroupState        current stage group state
     * @param flowLaunchLookup       flow lookup
     * @param cancelFlowConsumer     consumer that cancels provided flow
     * @param rescheduleFlowConsumer consumer to reschedule this check
     */
    void tryDisplacement(FlowLaunchId flowLaunchId,
                         StageGroupState stageGroupState,
                         Function<FlowLaunchId, FlowLaunchEntity> flowLaunchLookup,
                         Consumer<FlowLaunchEntity> cancelFlowConsumer,
                         Runnable rescheduleFlowConsumer) {
        log.info("Checking Flow Displacement for {}", flowLaunchId);

        var itemOpt = stageGroupState.getQueueItem(flowLaunchId);
        if (itemOpt.isEmpty()) {
            log.info("No Displacement required, queue item not found");
            return; // ---
        }

        var item = itemOpt.orElseThrow();
        log.info("Checking Queue Item: {}", item);

        var lockIntent = item.getLockIntent();
        if (lockIntent == null) {
            log.info("No Displacement required, no lock intent found");
            return; // ---
        }

        var nextItemOpt = stageGroupState.getNextQueueItem(item);
        if (nextItemOpt.isEmpty()) {
            log.info("No Displacement required, next queue item is empty");
            return; // ---
        }

        var currentFlowLaunch = flowLaunchLookup.apply(flowLaunchId);

        var displacementOptions = lookupStageDisplacementOptions(currentFlowLaunch, lockIntent);
        log.info("Displacement options: {}", displacementOptions);

        if (displacementOptions.isEmpty()) {
            log.info("No Displacement required, no supported options");
            return; // ---
        }

        var nextItem = nextItemOpt.orElseThrow();
        log.info("Checking next Queue Item: {}", nextItem);

        var nextFlowLaunchId = nextItem.getFlowLaunchId();
        var nextFlowLaunch = flowLaunchLookup.apply(nextFlowLaunchId);
        var nextStates = LaunchState.allFromStatistics(LaunchStatistics.fromLaunch(nextFlowLaunch, stageGroupState));
        log.info("Next Flow states: {}", nextStates);

        if (nextStates.isEmpty()) {
            log.info("No Displacement required, no states found");
            return; // ---
        }

        // Make sure next states are fully covered with accepted displacement options
        var uncoveredStates = new HashSet<>(nextStates);
        uncoveredStates.removeAll(displacementOptions);
        if (uncoveredStates.isEmpty()) {
            log.info("Accept Flow Displacement: {}", nextFlowLaunchId);
            Preconditions.checkState(!nextFlowLaunchId.equals(flowLaunchId), "Cannot cancel own flow");
            cancelFlowConsumer.accept(nextFlowLaunch);
        } else {
            // We could probably displace, but not right now
            // Must schedule delayed (periodic) operation to check this displacement
            log.info("No Displacement required yet, uncovered states: {}", uncoveredStates);
            rescheduleFlowConsumer.run();
        }
    }

    private Set<LaunchState> lookupStageDisplacementOptions(FlowLaunchEntity currentFlowLaunch,
                                                            StageGroupState.LockIntent lockIntent) {
        return lookupDisplacementOptions(currentFlowLaunch, lockIntent.getDesiredStageId());
    }

    private Set<LaunchState> lookupDisplacementOptions(FlowLaunchEntity currentFlowLaunch, String stageId) {
        var entity = currentFlowLaunch.getStage(new StageRefImpl(stageId));
        var options = entity.getDisplacementOptions();
        log.info("Stage {}, has displacement options: {}", stageId, options);
        return options;
    }


}
