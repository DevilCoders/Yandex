package ru.yandex.ci.flow.engine.runtime.state;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.flow.engine.definition.stage.StageRefImpl;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.StageGroupChangeEvent;
import ru.yandex.ci.flow.engine.runtime.events.UnlockStageEvent;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.FlowCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;

@Slf4j
public class StageGroupStateCalculator {

    private final StageGroupState stageGroupState;
    private final FlowLaunchEntity flowLaunch;

    private final List<FlowCommand> commands = new ArrayList<>();

    public StageGroupStateCalculator(StageGroupState stageGroupState, FlowLaunchEntity flowLaunch) {
        this.stageGroupState = stageGroupState;
        this.flowLaunch = flowLaunch;
    }

    public List<FlowCommand> getCommands() {
        return commands;
    }

    public void removeFromQueue(FlowLaunchId flowLaunchId) {
        Optional<StageGroupState.QueueItem> queueItem = stageGroupState.getQueueItem(flowLaunchId);

        if (queueItem.isEmpty()) {
            log.info("No queue item with flowLaunchId={}, nothing to remove", flowLaunchId);
            return;
        }

        Optional<StageGroupState.QueueItem> previousQueueItem = stageGroupState.getPreviousQueueItem(queueItem.get());
        stageGroupState.removeFromQueue(queueItem.get());
        previousQueueItem.ifPresent(this::updateQueueItem);
        ensureStateValid();
    }

    @SuppressWarnings("ReferenceEquality")
    public void unlockStage(String stageIdToUnlock, FlowLaunchId flowLaunchId) {
        StageGroupState.QueueItem queueItem = stageGroupState.getQueueItem(flowLaunchId)
                .orElseThrow(() -> new NoSuchElementException("No queue item with flowLaunchId=" + flowLaunchId));

        Preconditions.checkArgument(
                queueItem.getAcquiredStageIds().contains(stageIdToUnlock),
                "Flow doesn't hold the stage that it wants to release (%s), queueItem = %s",
                stageIdToUnlock, queueItem
        );

        queueItem.removeStageId(stageIdToUnlock);
        queueItem.resetIntent();

        for (StageGroupState.QueueItem item : Lists.reverse(stageGroupState.getQueue())) {
            if (item == queueItem) {
                continue;
            }

            if (item.getLockIntent() != null && stageIdToUnlock.equals(item.getDesiredStageId())) {
                StageGroupState.LockIntent lockIntent = item.getLockIntent();
                takeStage(item, lockIntent.getStageIdsToUnlock(), lockIntent.getDesiredStageId());
                break;
            }
        }

        Optional<StageGroupState.QueueItem> previousQueueItem = stageGroupState.getPreviousQueueItem(queueItem);
        previousQueueItem.ifPresent(this::updateQueueItem);
        ensureStateValid();

        commands.add(new FlowCommand(queueItem.getFlowLaunchId(), StageGroupChangeEvent.INSTANCE));
    }

    public void lockStage(String desiredStageId, FlowLaunchId flowLaunchId, boolean skipStagesAllowed) {
        tryLockStage(Collections.emptyList(), desiredStageId, flowLaunchId, skipStagesAllowed);
    }

    public void tryLockStage(Collection<String> stageIdsToUnlock, String desiredStageId, FlowLaunchId flowLaunchId,
                             boolean skipStagesAllowed) {
        StageGroupState.QueueItem queueItem = stageGroupState.getQueueItem(flowLaunchId)
                .orElseGet(() -> {
                    StageGroupState.QueueItem item = new StageGroupState.QueueItem(flowLaunchId);
                    log.info("Enqueue next item for stage group: {}", item);
                    stageGroupState.enqueue(item, desiredStageId, skipStagesAllowed);
                    return item;
                });
        log.info("Current stage group: {}", stageGroupState);
        log.info("Try Lock stage for {}", queueItem);

        Preconditions.checkArgument(
                queueItem.getAcquiredStageIds().containsAll(stageIdsToUnlock),
                "Flow doesn't hold the stages that it want to release (%s), queueItem = %s",
                stageIdsToUnlock, queueItem
        );

        Preconditions.checkArgument(
                !queueItem.getAcquiredStageIds().contains(desiredStageId)
                        || desiredStageId.equals(queueItem.getDesiredStageId()),
                "Flow already acquired stage %s, queueItem = %s",
                stageIdsToUnlock, queueItem
        );

        List<String> acquiredStageIds = stageGroupState.getNextQueueItems(queueItem).stream()
                .flatMap(item -> item.getAcquiredStageIds().stream())
                .distinct()
                .collect(Collectors.toList());

        var currentStages = flowLaunch.getStages().stream()
                .map(StoredStage::getId)
                .collect(Collectors.toList());

        boolean canTakeStageNow = acquiredStageIds.stream().allMatch(this.flowLaunch::hasStage);
        if (canTakeStageNow) {
            canTakeStageNow = acquiredStageIds.isEmpty()
                    || this.flowLaunch.isStageLessThanAll(desiredStageId, acquiredStageIds, true);
            if (!canTakeStageNow) {
                log.info("Can't take stage [{}] right now, UNFINISHED, acquired stages: {}, current flow stages: {}",
                        desiredStageId, acquiredStageIds, currentStages);
            }
        } else {
            log.info("Can't take stage [{}] right now, UNMATCHED, acquired stages: {}, current flow stages: {}",
                    desiredStageId, acquiredStageIds, currentStages);
        }

        if (canTakeStageNow) {
            log.info("Taking stage [{}] right now, unlocking: {}",
                    desiredStageId, stageIdsToUnlock);
            takeStage(queueItem, stageIdsToUnlock, desiredStageId);
        } else {
            queueItem.setIntent(stageIdsToUnlock, desiredStageId);
            log.info("Update Lock stage intent for {}", queueItem);
        }

        ensureStateValid();
    }

    private void takeStage(
            StageGroupState.QueueItem queueItem,
            @Nullable Collection<String> stageIdsToUnlock,
            String desiredStageId
    ) {
        queueItem.addStageId(desiredStageId);
        if (stageIdsToUnlock != null) {
            stageIdsToUnlock.forEach(
                    stageId -> commands.add(
                            new FlowCommand(
                                    queueItem.getFlowLaunchId(),
                                    new UnlockStageEvent(new StageRefImpl(stageId))
                            )
                    )
            );
        }

        commands.add(new FlowCommand(queueItem.getFlowLaunchId(), StageGroupChangeEvent.INSTANCE));
        stageGroupState.getPreviousQueueItem(queueItem).ifPresent(this::updateQueueItem);
    }

    private void updateQueueItem(StageGroupState.QueueItem queueItem) {
        if (queueItem.getLockIntent() != null && queueItem.getDesiredStageId() != null) {
            Optional<Set<String>> nextStageIds = stageGroupState.getNextQueueItem(queueItem)
                    .map(StageGroupState.QueueItem::getAcquiredStageIds);
            if (nextStageIds.isEmpty()
                    || this.flowLaunch.isStageLessThanAll(queueItem.getDesiredStageId(), nextStageIds.get(), true)) {
                StageGroupState.LockIntent lockIntent = queueItem.getLockIntent();
                takeStage(queueItem, lockIntent.getStageIdsToUnlock(), lockIntent.getDesiredStageId());
            }
        }
    }

    /**
     * Убеждается, что пересчёт стейта не привёл его в неконсистентное состояние.
     */
    private void ensureStateValid() {
        List<StageGroupState.QueueItem> queue = stageGroupState.getQueue();
        for (int i = 1; i < queue.size(); i++) {
            StageGroupState.QueueItem item = queue.get(i);

            List<StageGroupState.QueueItem> previousItems = queue.subList(0, i);
            Set<String> previouslyAcquiredStages = previousItems.stream()
                    .flatMap(it -> it.getAcquiredStageIds().stream())
                    .filter(this.flowLaunch::hasStage)
                    .collect(Collectors.toSet());

            /*
            Fail scenario:

            1) current flow - has stage [stage1, stage2], trying to acquire [stage1]
            2) previous flow 1 - has stage acquired [stage1] out of [stage1, stage2]
            3) previous flow 2 - has stage acquired [stage3] out of [stage1, stage2, stage3]

            Validate will fail because 'previouslyAcquiredStages' is [stage1, stage3] and current flow
            has no information about stage3.

            Skip all stages we have no information about
            */

            boolean isValid = item.getAcquiredStageIds().stream()
                    .allMatch(stageId ->
                            this.flowLaunch.isStageGreaterThanAll(stageId, previouslyAcquiredStages, false)
                    );

            Preconditions.checkState(
                    isValid,
                    "Recalculation violated invariant: " +
                            "previous stage must hold earlier stages, previous items = %s, current item = %s",
                    previousItems, item
            );
        }
    }

}
