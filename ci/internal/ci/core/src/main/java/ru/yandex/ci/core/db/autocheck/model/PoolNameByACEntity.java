package ru.yandex.ci.core.db.autocheck.model;

import java.time.Instant;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Builder
@Table(name = "autocheck/DistBuildPoolsByAC")
public class PoolNameByACEntity implements Entity<PoolNameByACEntity> {

    @Nonnull
    Id id;

    @Nonnull
    List<String> poolNames;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<PoolNameByACEntity> {
        @Column(dbType = DbType.TIMESTAMP)
        @Nonnull
        Instant updated;

        @Nonnull
        AccessControl ac;
    }
}
