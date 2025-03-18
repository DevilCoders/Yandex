package ru.yandex.ci.ydb.service.metric;

import java.time.Instant;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.DbTypeQualifier;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Table(name = "main/Metric")
public class Metric implements Entity<Metric> {

    Metric.Id id;

    double value;


    @Override
    public Metric.Id getId() {
        return id;
    }

    public Instant getTime() {
        return id.time;
    }

    public static Metric of(MetricId id, Instant time, double value) {
        return new Metric(Id.of(id.asString(), time), value);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<Metric> {
        @Column(name = "id", dbType = DbType.UTF8)
        String id;

        @Column(name = "timeSeconds", dbType = DbType.UINT64, dbTypeQualifier = DbTypeQualifier.SECONDS)
        Instant time;
    }

}
