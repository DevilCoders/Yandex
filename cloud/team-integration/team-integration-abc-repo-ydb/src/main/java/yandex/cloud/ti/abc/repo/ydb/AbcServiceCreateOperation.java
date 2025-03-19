package yandex.cloud.ti.abc.repo.ydb;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.db.Entity;

@Value
@Table(name = AbcServiceCloudCreateOperationReferenceEntity.TABLE_NAME)
class AbcServiceCreateOperation implements Entity<AbcServiceCreateOperation>, AbcServiceCloudCreateOperationReferenceEntity {

    Id id;

    @Column(name = OPERATION_ID_FIELD)
    @NotNull Operation.Id operationId;


    @Override
    public long getAbcServiceId() {
        return getId().getAbcId();
    }


    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AbcServiceCreateOperation> {

        @Column(name = ABC_SERVICE_ID_FIELD, dbType = ABC_SERVICE_ID_FIELD_TYPE)
        long abcId;

    }

}
