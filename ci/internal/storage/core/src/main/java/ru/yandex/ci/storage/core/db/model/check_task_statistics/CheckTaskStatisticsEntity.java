package ru.yandex.ci.storage.core.db.model.check_task_statistics;

import java.time.Instant;
import java.util.function.Function;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

@Value
@Builder(toBuilder = true)
@Table(name = "CheckTaskStatistics")
// This statistics is filled on message from main stream
// We can cache only values with id.reader == hostname
public class CheckTaskStatisticsEntity implements Entity<CheckTaskStatisticsEntity> {
    Id id;

    @Column(flatten = false)
    CheckTaskStatistics statistics;

    @Column(dbType = DbType.TIMESTAMP)
    Instant timestamp;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<CheckTaskStatisticsEntity> {
        CheckTaskEntity.Id taskId;
        int partition; // to avoid concurrency between partitions
        String reader; // to avoid concurrency between readers
    }

    public static Function<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity> defaultProvider() {
        return id -> CheckTaskStatisticsEntity.builder().id(id).statistics(CheckTaskStatistics.EMPTY).build();
    }
}
