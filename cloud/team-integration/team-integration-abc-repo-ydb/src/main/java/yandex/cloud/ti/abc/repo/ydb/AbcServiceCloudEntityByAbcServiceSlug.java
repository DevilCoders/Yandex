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
@Table(name = AbcServiceCloudEntity.TABLE_NAME + "_slug_index")
class AbcServiceCloudEntityByAbcServiceSlug implements IndexEntity<AbcServiceCloudEntityByCloudId, AbcServiceCloudEntityByAbcServiceSlug>, AbcServiceCloudEntity {

    @Column(name = ABC_SERVICE_SLUG_FIELD)
    @NotNull Id id;

    @Column(name = CLOUD_ID_FIELD, dbType = CLOUD_ID_FIELD_TYPE)
    @NotNull String cloudId;

    @Column(name = DEFAULT_FOLDER_ID_FIELD, dbType = DEFAULT_FOLDER_ID_FIELD_TYPE)
    @NotNull String defaultFolderId;

    @Column(name = ABC_SERVICE_ID_FIELD, dbType = ABC_SERVICE_ID_FIELD_TYPE)
    long abcId;

    @Column(name = ABC_FOLDER_ID_FIELD, dbType = ABC_FOLDER_ID_FIELD_TYPE)
    @NotNull String abcFolderId;

    @Column(name = CREATED_AT_FIELD)
    @NotNull Instant createdAt;

    @Override
    public @NotNull String getAbcSlug() {
        return id.getAbcSlug();
    }


    @Override
    public AbcServiceCloudEntityByCloudId toEntity() {
        return AbcServiceCloudEntityByCloudId.fromEntity(this);
    }

    public static @NotNull AbcServiceCloudEntityByAbcServiceSlug fromEntity(@NotNull AbcServiceCloudEntity entity) {
        return new AbcServiceCloudEntityByAbcServiceSlug(
                Id.fromEntity(entity),
                entity.getCloudId(),
                entity.getDefaultFolderId(),
                entity.getAbcId(),
                entity.getAbcFolderId(),
                entity.getCreatedAt()
        );
    }


    @Value(staticConstructor = "of")
    public static class Id implements IndexEntity.Id<AbcServiceCloudEntityByAbcServiceSlug> {

        @Column(name = ABC_SERVICE_SLUG_FIELD, dbType = ABC_SERVICE_SLUG_FIELD_TYPE)
        @NotNull String abcSlug;

        public static @NotNull Id fromEntity(@NotNull AbcServiceCloudEntity entity) {
            return new Id(
                    entity.getAbcSlug()
            );
        }

    }

}
