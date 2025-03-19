package yandex.cloud.ti.abc.repo.ydb;

import java.time.Instant;
import java.util.List;

import lombok.AllArgsConstructor;
import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@AllArgsConstructor
@Table(name = AbcServiceCloudEntity.TABLE_NAME)
class AbcServiceCloudEntityByCloudId implements Entity<AbcServiceCloudEntityByCloudId>, AbcServiceCloudEntity {

    @Column(name = CLOUD_ID_FIELD)
    @NotNull Id id;

    @Column(name = DEFAULT_FOLDER_ID_FIELD, dbType = DEFAULT_FOLDER_ID_FIELD_TYPE)
    @NotNull String defaultFolderId;

    @Column(name = ABC_SERVICE_ID_FIELD, dbType = ABC_SERVICE_ID_FIELD_TYPE)
    long abcId;

    @Column(name = ABC_SERVICE_SLUG_FIELD, dbType = ABC_SERVICE_SLUG_FIELD_TYPE)
    @NotNull String abcSlug;

    @Column(name = ABC_FOLDER_ID_FIELD, dbType = ABC_FOLDER_ID_FIELD_TYPE)
    @NotNull String abcFolderId;

    @Column(name = CREATED_AT_FIELD)
    @NotNull Instant createdAt;


    @Override
    public @NotNull String getCloudId() {
        return id.getCloudId();
    }

    @Override
    public List<Entity<?>> createProjections() {
        return List.of(
                AbcServiceCloudEntityByAbcServiceId.fromEntity(this),
                AbcServiceCloudEntityByAbcdFolderId.fromEntity(this),
                AbcServiceCloudEntityByAbcServiceSlug.fromEntity(this)
        );
    }

    public static @NotNull AbcServiceCloudEntityByCloudId fromEntity(@NotNull AbcServiceCloudEntity entity) {
        return new AbcServiceCloudEntityByCloudId(
                Id.fromEntity(entity),
                entity.getDefaultFolderId(),
                entity.getAbcId(),
                entity.getAbcSlug(),
                entity.getAbcFolderId(),
                entity.getCreatedAt()
        );
    }


    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AbcServiceCloudEntityByCloudId> {

        @Column(name = CLOUD_ID_FIELD, dbType = CLOUD_ID_FIELD_TYPE)
        @NotNull String cloudId;

        public static @NotNull Id fromEntity(@NotNull AbcServiceCloudEntity entity) {
            return new Id(
                    entity.getCloudId()
            );
        }

    }

}
