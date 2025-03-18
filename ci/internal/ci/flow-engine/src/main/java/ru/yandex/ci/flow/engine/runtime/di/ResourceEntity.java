package ru.yandex.ci.flow.engine.runtime.di;

import java.time.Instant;
import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceModification;
import ru.yandex.ci.ydb.Persisted;

@Value
@Builder(toBuilder = true)
@Table(name = "flow/Resource")
@GlobalIndex(name = ResourceEntity.IDX_FLOW_LAUNCH_ID, fields = {"flowLaunchId"})
public class ResourceEntity implements Entity<ResourceEntity> {

    public static final String IDX_FLOW_LAUNCH_ID = "IDX_FLOW_LAUNCH_ID";

    @Nonnull
    ResourceEntity.Id id;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    String flowId;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    String flowLaunchId;

    @Nonnull
    @Column(dbType = DbType.JSON, flatten = false)
    List<StoredResourceModification> modifications;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    String resourceType;

    boolean classBased;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    String sourceCodeId;

    @Nonnull
    @Column(dbType = DbType.JSON, flatten = false)
    JsonObject object;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant updated;

    @Nullable
    @Column(dbType = DbType.STRING)
    ResourceClass resourceClass;

    @Nonnull
    @Override
    public ResourceEntity.Id getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<ResourceEntity> {

        @Nonnull
        @Column(name = "id", dbType = DbType.UTF8)
        String id;
    }

    @Persisted
    public enum ResourceClass {
        // Статические ресурсы конфигурируются на этапе создания нового flow, динамического создания задач и т.д.
        STATIC,

        // В будущем это будут ручные артефакты, создаваемые при старте flow
        MANUAL,

        @Deprecated
        SUBSCRIBER,

        // Обычные динамические артефакты - результат работы кубика
        DYNAMIC
    }
}
