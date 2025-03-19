package yandex.cloud.ti.abc.repo.ydb;

import java.time.Instant;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.ti.repository.TeamIntegrationRepositoryProperties;

interface AbcServiceCloudEntity {

    @NotNull String TABLE_NAME = TeamIntegrationRepositoryProperties.TABLE_NAME_PREFIX + "abc_services";

    @NotNull String ABC_SERVICE_ID_FIELD = "abc_id";
    @NotNull String ABC_SERVICE_ID_FIELD_TYPE = DbType.INT64;

    @NotNull String ABC_SERVICE_SLUG_FIELD = "abc_slug";
    @NotNull String ABC_SERVICE_SLUG_FIELD_TYPE = DbType.UTF8;

    @NotNull String ABC_FOLDER_ID_FIELD = "abc_folder_id";
    @NotNull String ABC_FOLDER_ID_FIELD_TYPE = DbType.UTF8;

    @NotNull String CLOUD_ID_FIELD = "cloud_id";
    @NotNull String CLOUD_ID_FIELD_TYPE = DbType.UTF8;

    @NotNull String DEFAULT_FOLDER_ID_FIELD = "default_folder_id";
    @NotNull String DEFAULT_FOLDER_ID_FIELD_TYPE = DbType.UTF8;

    @NotNull String CREATED_AT_FIELD = "created_at";


    long getAbcId();

    @NotNull String getAbcSlug();

    @NotNull String getAbcFolderId();

    @NotNull String getCloudId();

    @NotNull String getDefaultFolderId();

    @NotNull Instant getCreatedAt();

}
