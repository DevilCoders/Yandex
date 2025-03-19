package yandex.cloud.iam.repository.operation;

import yandex.cloud.audit.OperationDb;
import yandex.cloud.model.operation.EntityOperation;
import yandex.cloud.repository.db.AbstractDelegatingTable;
import yandex.cloud.repository.db.AbstractTable;

@Deprecated
public class EntityOperationTableImpl
        extends AbstractDelegatingTable<EntityOperation>
        implements OperationDb.EntityOperationTable {

    public EntityOperationTableImpl(AbstractTable<EntityOperation> table) {
        super(table);
    }
}
