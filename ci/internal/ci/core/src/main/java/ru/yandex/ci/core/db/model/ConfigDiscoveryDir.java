package ru.yandex.ci.core.db.model;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

@SuppressWarnings({"BoxedPrimitiveEquality", "ReferenceEquality"})
@Value(staticConstructor = "of")
@Table(name = "main/ConfigDiscoveryDir")
@GlobalIndex(name = ConfigDiscoveryDir.IDX_PATH_PREFIX, fields = "id.pathPrefix")
public class ConfigDiscoveryDir implements Entity<ConfigDiscoveryDir> {

    public static final String IDX_PATH_PREFIX = "IDX_PATH_PREFIX";

    @Nonnull
    @Column
    ConfigDiscoveryDir.Id id;

    @Nullable
    @Column(dbType = DbType.STRING)
    String commitId;

    @With
    @Nullable
    Boolean deleted;

    @Override
    public Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ConfigDiscoveryDir> {
        @Nonnull
        @Column(dbType = DbType.STRING)
        String configPath;

        @Nonnull
        @Column(dbType = DbType.STRING)
        String pathPrefix;
    }

}
