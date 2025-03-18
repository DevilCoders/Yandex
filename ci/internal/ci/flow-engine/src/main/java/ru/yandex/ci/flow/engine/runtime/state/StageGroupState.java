package ru.yandex.ci.flow.engine.runtime.state;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.definition.stage.StageRefImpl;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.ydb.Persisted;

/**
 * Снэпшот состояния стадий синхронизации. Хранит очередь флоу и залоченные ими стадии.
 */
@Value
@Table(name = "flow/StageGroup")
public class StageGroupState implements Entity<StageGroupState> {
    @Nonnull
    @Column
    StageGroupState.Id id;

    @With
    @Column
    long version;

    @Nonnull
    @Column
    List<QueueItem> queue;

    @Nonnull
    @Override
    public StageGroupState.Id getId() {
        return id;
    }

    public Optional<QueueItem> getQueueItem(FlowLaunchId flowLaunchId) {
        return queue.stream().filter(item -> item.getFlowLaunchId().equals(flowLaunchId)).findFirst();
    }

    public void enqueue(QueueItem item) {
        enqueue(item, null, false);
    }

    public void enqueue(QueueItem item, @Nullable String desiredStageId, boolean skipStagesAllowed) {
        if (!skipStagesAllowed) {
            queue.add(0, item);
        } else {
            QueueItem itemHoldingDesiredStage = queue.stream()
                    .filter(x -> x.acquiredStageIds.contains(desiredStageId))
                    .findFirst().orElse(null);
            if (itemHoldingDesiredStage == null) {
                queue.add(item);
            } else {
                queue.add(queue.indexOf(itemHoldingDesiredStage), item);
            }
        }
    }

    public Optional<QueueItem> getNextQueueItem(QueueItem queueItem) {
        int index = queue.indexOf(queueItem);
        return index + 1 < queue.size() ? Optional.of(queue.get(index + 1)) : Optional.empty();
    }

    public List<QueueItem> getNextQueueItems(QueueItem queueItem) {
        int index = queue.indexOf(queueItem);
        return index + 1 < queue.size() ? queue.subList(index + 1, queue.size()) : Collections.emptyList();
    }

    public Optional<QueueItem> getPreviousQueueItem(QueueItem queueItem) {
        int index = queue.indexOf(queueItem);
        return index - 1 >= 0 ? Optional.of(queue.get(index - 1)) : Optional.empty();
    }

    public List<QueueItem> getPreviousQueueItems(QueueItem queueItem) {
        int index = queue.indexOf(queueItem);
        return index >= 0 ? queue.subList(0, index) : Collections.emptyList();
    }

    public void removeFromQueue(QueueItem queueItem) {
        queue.remove(queueItem);
    }


    public boolean isStageFree(String stageId) {
        return queue.stream().noneMatch(item -> item.getAcquiredStageIds().contains(stageId));
    }

    public boolean isStageAcquiredBy(FlowLaunchId flowLaunchId, StageRef stage) {
        return getQueueItem(flowLaunchId).map(item -> item.getAcquiredStageIds().contains(stage.getId())).orElse(false);
    }

    public Set<? extends StageRef> getAcquiredStages(FlowLaunchId flowLaunchId) {
        return getQueueItem(flowLaunchId)
                .map(
                        item -> item.getAcquiredStageIds().stream()
                                .map(StageRefImpl::new)
                                .collect(Collectors.toSet())
                )
                .orElse(Collections.emptySet());
    }

    public Set<String> getAcquiredStagesIds(FlowLaunchId flowLaunchId) {
        return getQueueItem(flowLaunchId)
                .map(QueueItem::getAcquiredStageIds)
                .orElse(Collections.emptySet());
    }

    //

    public static StageGroupState of(String id) {
        return of(id, 0);
    }

    public static StageGroupState of(String id, long version) {
        return new StageGroupState(Id.of(id), version, new ArrayList<>());
    }

    @VisibleForTesting
    public static StageGroupState from(String id, long version, List<QueueItem> queue) {
        return new StageGroupState(Id.of(id), version, queue);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<StageGroupState> {
        @Column(dbType = DbType.UTF8)
        String id;
    }

    @Persisted
    @Data
    @AllArgsConstructor
    public static class QueueItem {
        @Nonnull
        private final FlowLaunchId flowLaunchId;

        @Nonnull
        private final Set<String> acquiredStageIds;

        @Nullable
        private LockIntent lockIntent;

        public QueueItem(@Nonnull FlowLaunchId flowLaunchId) {
            this.flowLaunchId = flowLaunchId;
            this.acquiredStageIds = new HashSet<>();
        }

        @Nonnull
        public Set<String> getAcquiredStageIds() {
            return acquiredStageIds;
        }

        public void addStageId(String stageId) {
            acquiredStageIds.add(stageId);
            lockIntent = null;
        }

        public void removeStageId(String stageId) {
            acquiredStageIds.remove(stageId);
        }

        @Nullable
        public String getDesiredStageId() {
            return lockIntent != null ? lockIntent.desiredStageId : null;
        }

        public void setIntent(Collection<String> stageIdsToUnlock, String desiredStageId) {
            lockIntent = new LockIntent(desiredStageId, stageIdsToUnlock);
        }

        public void resetIntent() {
            lockIntent = null;
        }

    }

    @Persisted
    @Value
    public static class LockIntent {
        @Nonnull
        String desiredStageId;

        @Nonnull
        Collection<String> stageIdsToUnlock;
    }
}
