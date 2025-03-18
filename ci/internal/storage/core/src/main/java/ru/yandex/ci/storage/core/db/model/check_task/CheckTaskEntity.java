package ru.yandex.ci.storage.core.db.model.check_task;

import java.time.Instant;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "CheckTasks")
@GlobalIndex(name = CheckTaskEntity.IDX_BY_STATUS_AND_RIGHT, fields = {"status", "right"})
@With
public class CheckTaskEntity implements Entity<CheckTaskEntity> {
    public static final String IDX_BY_STATUS_AND_RIGHT = "IDX_BY_STATUS_AND_RIGHT";

    Id id;

    @Nullable
    Common.CheckTaskType type;

    int numberOfPartitions;

    boolean right;
    CheckStatus status;

    String jobName;

    Set<Integer> completedPartitions;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    Instant finish;

    @Nullable // old values
    PartitionsFinishStatistics partitionsStatistics;

    @Nullable
    Map<Integer, TechnicalStatistics> partitionsTechnicalStatistics;

    @Nullable
    Map<Integer, Metrics> partitionsMetrics;

    public Map<Integer, TechnicalStatistics> getPartitionsTechnicalStatistics() {
        return Objects.requireNonNullElse(partitionsTechnicalStatistics, Map.of());
    }

    public Map<Integer, Metrics> getPartitionsMetrics() {
        return Objects.requireNonNullElse(partitionsMetrics, Map.of());
    }

    public PartitionsFinishStatistics getPartitionsStatistics() {
        return Objects.requireNonNullElse(partitionsStatistics, PartitionsFinishStatistics.EMPTY);
    }

    @Override
    public Id getId() {
        return id;
    }

    public Common.CheckTaskType getType() {
        return Objects.requireNonNullElse(type, Common.CheckTaskType.CTT_AUTOCHECK);
    }

    public CheckTaskEntity complete(CheckStatus status) {
        return this.toBuilder()
                .status(status)
                .finish(Instant.now())
                .build();
    }

    public String getLogbrokerSourceId() {
        return id.getIterationId().getCheckId().toString() +
                "/" + id.getIterationId().getIterationType().name() +
                "/" + id.getIterationId().getNumber() +
                "/" + id.getTaskId();
    }

    public boolean isLeft() {
        return !right;
    }

    @Value
    @lombok.Builder
    @AllArgsConstructor
    public static class Id implements Entity.Id<CheckTaskEntity> {
        CheckIterationEntity.Id iterationId;
        String taskId;

        @Override
        public String toString() {
            return "[" + iterationId + "/" + taskId + "]";
        }
    }
}
