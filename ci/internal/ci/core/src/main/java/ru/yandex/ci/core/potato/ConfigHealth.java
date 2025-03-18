package ru.yandex.ci.core.potato;

import java.time.Instant;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@Value
@Builder
@Table(name = "main/ConfigHealth")
public class ConfigHealth implements Entity<ConfigHealth> {

    ConfigHealth.Id id;

    @Column
    boolean healthy;

    @Column(dbType = DbType.TIMESTAMP)
    Instant lastSeen;

    @Override
    public ConfigHealth.Id getId() {
        return id;
    }

    @Value
    @lombok.Builder
    public static class Id implements Entity.Id<ConfigHealth> {
        @Column(dbType = DbType.STRING)
        String configType;

        @Column(dbType = DbType.STRING)
        String vcs;

        @Column(dbType = DbType.STRING)
        String project;

        @Column(dbType = DbType.STRING)
        String repo;
    }
}
