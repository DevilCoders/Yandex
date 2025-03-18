package ru.yandex.ci.core.registry;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.config.registry.RollbackMode;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;

@Value
@Builder
@Table(name = "flow/RegistryTask")
public class RegistryTask implements Entity<RegistryTask> {
    Id id;

    @Nonnull
    @Column(dbType = DbType.STRING)
    Type type;

    @Nonnull
    @Column(dbType = DbType.STRING)
    RollbackMode rollbackMode;

    @Nullable
    @Column(flatten = false)
    TaskletMetadata.Id taskletMetadataId;

    @Nullable
    @Column(flatten = false)
    TaskletV2Metadata.Description taskletV2MetadataDescription;

    @Nullable
    @Column(dbType = DbType.STRING)
    String sandboxTaskName;

    @Nullable
    @Column(dbType = DbType.STRING)
    String sandboxTemplateName;

    boolean stale;

    @Value
    public static class Id implements Entity.Id<RegistryTask> {
        @Column(dbType = DbType.STRING)
        String task;

        @Column(dbType = DbType.STRING)
        String version;

        public static Id of(TaskId task, TaskVersion version) {
            return new Id(task.getId(), version.getValue());
        }
    }

    @Override
    public Id getId() {
        return id;
    }
}
