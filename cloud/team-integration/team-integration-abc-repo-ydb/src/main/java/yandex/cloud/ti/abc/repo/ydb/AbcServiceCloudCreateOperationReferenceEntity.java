package yandex.cloud.ti.abc.repo.ydb;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.ti.repository.TeamIntegrationRepositoryProperties;

interface AbcServiceCloudCreateOperationReferenceEntity {

    @NotNull String TABLE_NAME = TeamIntegrationRepositoryProperties.TABLE_NAME_PREFIX + "abc_service_create_operations";

    @NotNull String ABC_SERVICE_ID_FIELD = "abc_id";
    @NotNull String ABC_SERVICE_ID_FIELD_TYPE = DbType.INT64;

    @NotNull String OPERATION_ID_FIELD = "operation_id";


    long getAbcServiceId();

    @NotNull Operation.Id getOperationId();

}
