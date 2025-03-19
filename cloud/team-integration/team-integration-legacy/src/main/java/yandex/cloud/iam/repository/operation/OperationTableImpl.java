package yandex.cloud.iam.repository.operation;

import java.util.List;

import yandex.cloud.audit.OperationDb.OperationTable;
import yandex.cloud.binding.expression.FilterBuilder;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.db.AbstractDelegatingTable;
import yandex.cloud.repository.db.AbstractTable;
import yandex.cloud.repository.db.EntitySchema;

@Deprecated
public class OperationTableImpl
        extends AbstractDelegatingTable<Operation>
        implements OperationTable {

    public OperationTableImpl(AbstractTable<Operation> target) {
        super(target);
    }

    @Override
    public List<Operation> findByRootId(Operation.Id rootId) {
        var rootIdFilter = FilterBuilder.forSchema(EntitySchema.of(Operation.class))
                .where("rootId").eq(rootId)
                .build();

        // XXX findByRootId should never used, this is deprecated Operation class

        return find(null, rootIdFilter, null, 1000, 0L);
    }

}
