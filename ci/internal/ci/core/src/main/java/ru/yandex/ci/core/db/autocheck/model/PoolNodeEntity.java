package ru.yandex.ci.core.db.autocheck.model;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Builder
@Table(name = "autocheck/DistBuildPoolNodes")
public class PoolNodeEntity implements Entity<PoolNodeEntity> {

    @Nonnull
    Id id;

    @Column(dbType = DbType.INT32)
    int resourceGuarantiesSlots;

    @Column(dbType = DbType.DOUBLE)
    double weight;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<PoolNodeEntity> {
        @Column(dbType = DbType.TIMESTAMP)
        @Nonnull
        Instant updated;

        @Column(dbType = DbType.UTF8)
        @Nonnull
        String name;
    }
}
