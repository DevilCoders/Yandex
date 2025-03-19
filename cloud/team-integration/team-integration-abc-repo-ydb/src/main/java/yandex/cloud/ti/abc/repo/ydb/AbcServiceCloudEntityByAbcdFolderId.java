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
@Table(name = AbcServiceCloudEntity.TABLE_NAME + "_folder_index")
class AbcServiceCloudEntityByAbcdFolderId implements IndexEntity<AbcServiceCloudEntityByCloudId, AbcServiceCloudEntityByAbcdFolderId>, AbcServiceCloudEntity {

    @Column(name = ABC_FOLDER_ID_FIELD)
    @NotNull Id id;

    @Column(name = CLOUD_ID_FIELD, dbType = CLOUD_ID_FIELD_TYPE)
    @NotNull String cloudId;

    @Column(name = DEFAULT_FOLDER_ID_FIELD, dbType = DEFAULT_FOLDER_ID_FIELD_TYPE)
    @NotNull String defaultFolderId;

    @Column(name = ABC_SERVICE_ID_FIELD, dbType = ABC_SERVICE_ID_FIELD_TYPE)
    long abcId;

    @Column(name = ABC_SERVICE_SLUG_FIELD, dbType = ABC_SERVICE_SLUG_FIELD_TYPE)
    @NotNull String abcSlug;

    @Column(name = CREATED_AT_FIELD)
    @NotNull Instant createdAt;


    @Override
    public @NotNull String getAbcFolderId() {
        return id.getAbcFolderId();
    }

    @Override
    public AbcServiceCloudEntityByCloudId toEntity() {
        return AbcServiceCloudEntityByCloudId.fromEntity(this);
    }

    public static @NotNull AbcServiceCloudEntityByAbcdFolderId fromEntity(@NotNull AbcServiceCloudEntity entity) {
        return new AbcServiceCloudEntityByAbcdFolderId(
                Id.fromEntity(entity),
                entity.getCloudId(),
                entity.getDefaultFolderId(),
                entity.getAbcId(),
                entity.getAbcSlug(),
                entity.getCreatedAt()
        );
    }


    @Value(staticConstructor = "of")
    public static class Id implements IndexEntity.Id<AbcServiceCloudEntityByAbcdFolderId> {

        @Column(name = ABC_FOLDER_ID_FIELD, dbType = ABC_FOLDER_ID_FIELD_TYPE)
        @NotNull String abcFolderId;

        public static @NotNull Id fromEntity(@NotNull AbcServiceCloudEntity entity) {
            return new Id(
                    entity.getAbcFolderId()
            );
        }

    }

}
