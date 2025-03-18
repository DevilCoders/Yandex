package ru.yandex.ci.observer.core.db.model.check_tasks;

import java.time.Instant;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.traces.TimestampedTraceStages;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "CheckTasks")
@GlobalIndex(name = CheckTaskEntity.IDX_BY_RIGHT_REVISION_TIMESTAMP_CHECK_TYPE_STATUS_ITER_TYPE_JOB_NAME,
        fields = {"rightRevisionTimestamp", "checkType", "status", "id.iterationId.iterType", "jobName"})
public class CheckTaskEntity implements Entity<CheckTaskEntity> {

    public static final String IDX_BY_RIGHT_REVISION_TIMESTAMP_CHECK_TYPE_STATUS_ITER_TYPE_JOB_NAME =
            "IDX_BY_RIGHT_REVISION_TIMESTAMP_CHECK_TYPE_STATUS_ITER_TYPE_JOB_NAME";

    Id id;

    @Nullable   // null for old values
    CheckOuterClass.CheckType checkType;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    Instant finish;

    @Nullable   // null for old values
    @Column(dbType = DbType.TIMESTAMP)
    Instant rightRevisionTimestamp;

    int numberOfPartitions;

    boolean right;
    CheckStatus status;

    String jobName;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    Map<String, String> taskAttributes;

    Set<Integer> completedPartitions;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    Map<Integer, TimestampedTraceStages> timestampedStagesByPartitions;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    TimestampedTraceStages timestampedStagesAggregation;

    @Nullable
    Map<Integer, TechnicalStatistics> partitionsTechnicalStatistics;

    @Override
    public Id getId() {
        return id;
    }

    public Map<Integer, TechnicalStatistics> getPartitionsTechnicalStatistics() {
        return partitionsTechnicalStatistics == null ? Map.of() : partitionsTechnicalStatistics;
    }

    public Map<String, String> getTaskAttributes() {
        return taskAttributes == null ? Map.of() : taskAttributes;
    }

    public static class Builder {
        public CheckTaskEntity build() {
            return new CheckTaskEntity(
                    Objects.requireNonNull(id, "id is null"),
                    checkType,
                    Objects.requireNonNull(created, "created is null"),
                    finish,
                    rightRevisionTimestamp,
                    numberOfPartitions,
                    right,
                    Objects.requireNonNullElse(status, CheckStatus.CREATED),
                    Objects.requireNonNull(jobName, "jobName is null"),
                    Objects.requireNonNullElseGet(taskAttributes, HashMap::new),
                    Objects.requireNonNullElse(completedPartitions, Set.of()),
                    Objects.requireNonNullElseGet(timestampedStagesByPartitions, HashMap::new),
                    Objects.requireNonNullElseGet(timestampedStagesAggregation, TimestampedTraceStages::new),
                    Objects.requireNonNullElseGet(partitionsTechnicalStatistics, HashMap::new)
            );
        }
    }

    @Value
    public static class Id implements Entity.Id<CheckTaskEntity> {
        CheckIterationEntity.Id iterationId;
        String taskId;

        @Override
        public String toString() {
            return "[%s/%s]".formatted(iterationId, taskId);
        }
    }
}
