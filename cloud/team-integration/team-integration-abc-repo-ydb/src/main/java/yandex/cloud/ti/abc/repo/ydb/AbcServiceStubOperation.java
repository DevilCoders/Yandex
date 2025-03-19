package yandex.cloud.ti.abc.repo.ydb;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = AbcServiceCloudStubOperationReferenceEntity.TABLE_NAME)
class AbcServiceStubOperation implements Entity<AbcServiceStubOperation>, AbcServiceCloudStubOperationReferenceEntity {

    Id id;

    @Column(name = MAIN_OPERATION_ID_FIELD, dbType = MAIN_OPERATION_ID_FIELD_TYPE)
    Operation.Id mainOperationId;


    @Override
    public @NotNull Operation.Id getStubOperationId() {
        return getId().getOperationId();
    }


    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AbcServiceStubOperation> {

        @Column(name = ID_FIELD, dbType = ID_FIELD_TYPE)
        Operation.Id operationId;

    }

}
