package yandex.cloud.ti.abc.repo.ydb;

import java.time.Instant;

import lombok.AllArgsConstructor;
import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.iam.repository.IndexEntity;

@Value
@AllArgsConstructor
@Table(name = AbcServiceCloudEntity.TABLE_NAME + "_id_index")
class AbcServiceCloudEntityByAbcServiceId implements IndexEntity<AbcServiceCloudEntityByCloudId, AbcServiceCloudEntityByAbcServiceId>, AbcServiceCloudEntity {

    @Column(name = ABC_SERVICE_ID_FIELD)
    @NotNull Id id;

    @Column(name = CLOUD_ID_FIELD, dbType = CLOUD_ID_FIELD_TYPE)
    @NotNull String cloudId;

    @Column(name = DEFAULT_FOLDER_ID_FIELD, dbType = DEFAULT_FOLDER_ID_FIELD_TYPE)
    @NotNull String defaultFolderId;

    @Column(name = ABC_SERVICE_SLUG_FIELD, dbType = ABC_SERVICE_SLUG_FIELD_TYPE)
    @NotNull String abcSlug;

    @Column(name = ABC_FOLDER_ID_FIELD, dbType = ABC_FOLDER_ID_FIELD_TYPE)
    @NotNull String abcFolderId;

    @Column(name = CREATED_AT_FIELD)
    @NotNull Instant createdAt;


    @Override
    public long getAbcId() {
        return id.getAbcId();
    }

    @Override
    public AbcServiceCloudEntityByCloudId toEntity() {
        return AbcServiceCloudEntityByCloudId.fromEntity(this);
    }

    public static @NotNull AbcServiceCloudEntityByAbcServiceId fromEntity(@NotNull AbcServiceCloudEntity entity) {
        return new AbcServiceCloudEntityByAbcServiceId(
                Id.fromEntity(entity),
                entity.getCloudId(),
                entity.getDefaultFolderId(),
                entity.getAbcSlug(),
                entity.getAbcFolderId(),
                entity.getCreatedAt()
        );
    }


    @Value(staticConstructor = "of")
    public static class Id implements IndexEntity.Id<AbcServiceCloudEntityByAbcServiceId> {

        @Column(name = ABC_SERVICE_ID_FIELD, dbType = ABC_SERVICE_ID_FIELD_TYPE)
        long abcId;

        public static @NotNull Id fromEntity(@NotNull AbcServiceCloudEntity entity) {
            return new Id(
                    entity.getAbcId()
            );
        }

    }

}
