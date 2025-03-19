package yandex.cloud.ti.abc.repo.ydb;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.ti.repository.TeamIntegrationRepositoryProperties;

interface AbcServiceCloudStubOperationReferenceEntity {

    @NotNull String TABLE_NAME = TeamIntegrationRepositoryProperties.TABLE_NAME_PREFIX + "abc_service_stub_operations";

    @NotNull String ID_FIELD = "id";
    @NotNull String ID_FIELD_TYPE = DbType.UTF8;

    @NotNull String MAIN_OPERATION_ID_FIELD = "main_operation_id";
    @NotNull String MAIN_OPERATION_ID_FIELD_TYPE = DbType.UTF8;


    @NotNull Operation.Id getStubOperationId();

    @NotNull Operation.Id getMainOperationId();

}
