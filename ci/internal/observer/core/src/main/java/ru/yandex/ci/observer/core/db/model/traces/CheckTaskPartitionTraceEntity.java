package ru.yandex.ci.observer.core.db.model.traces;

import java.time.Instant;
import java.util.Map;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;

@Value
@Builder(toBuilder = true)
@Table(name = "CheckTasksPartitionTraces")
public class CheckTaskPartitionTraceEntity implements Entity<CheckTaskPartitionTraceEntity> {
    Id id;

    @Column(dbType = DbType.TIMESTAMP)
    Instant time;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    Map<String, String> attributes;

    @Override
    public Id getId() {
        return id;
    }

    public Map<String, String> getAttributes() {
        return attributes == null ? Map.of() : attributes;
    }

    @Value
    public static class Id implements Entity.Id<CheckTaskPartitionTraceEntity> {
        public static final int ALL_PARTITIONS = -1;

        CheckTaskEntity.Id taskId;
        int partition;
        String traceType;

        @Override
        public String toString() {
            return "[%s/%s/%s]".formatted(taskId, partition, traceType);
        }
    }
}
