package ru.yandex.ci.observer.core.db.model.sla_statistics;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Value
@Builder(toBuilder = true)
@Table(name = "SlaStatistics")
public class SlaStatisticsEntity implements Entity<SlaStatisticsEntity> {
    Id id;

    long avgIterationNumber;
    long avgIterationNumberWithRobots;

    @Column(dbType = DbType.JSON, flatten = false)
    Map<Integer, Long> durationSecondsPercentiles;

    @Nullable
    String advisedPool;
    String authors;
    @Nullable
    Long totalNumberOfNodes;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<SlaStatisticsEntity> {
        CheckOuterClass.CheckType checkType;
        IterationTypeGroup iterationTypeGroup;
        IterationCompleteGroup status;
        int windowDays;
        @Column(dbType = DbType.TIMESTAMP)
        Instant day;

        String uuid;

        public static Id create(
                CheckOuterClass.CheckType checkType,
                IterationTypeGroup iterationTypeGroup,
                IterationCompleteGroup status,
                int windowDays,
                Instant day
        ) {
            return new Id(checkType, iterationTypeGroup, status, windowDays, day, UUID.randomUUID().toString());
        }

        @Override
        public String toString() {
            return "[%s/%s/%s/%d/%s/%s]".formatted(checkType, iterationTypeGroup, status, windowDays, day, uuid);
        }
    }
}
