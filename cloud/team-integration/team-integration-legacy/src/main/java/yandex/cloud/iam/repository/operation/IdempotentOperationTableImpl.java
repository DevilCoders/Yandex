package yandex.cloud.iam.repository.operation;

import yandex.cloud.audit.OperationDb.IdempotentOperationTable;
import yandex.cloud.model.operation.IdempotentOperation;
import yandex.cloud.repository.db.AbstractDelegatingTable;
import yandex.cloud.repository.db.AbstractTable;

@Deprecated
public class IdempotentOperationTableImpl
        extends AbstractDelegatingTable<IdempotentOperation>
        implements IdempotentOperationTable {

    public IdempotentOperationTableImpl(AbstractTable<IdempotentOperation> table) {
        super(table);
    }
}
