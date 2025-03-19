package yandex.cloud.team.integration.repository;

import yandex.cloud.audit.OperationDb;
import yandex.cloud.iam.repository.operation.EntityOperationTableImpl;
import yandex.cloud.iam.repository.operation.IdempotentOperationTableImpl;
import yandex.cloud.iam.repository.operation.OperationTableImpl;
import yandex.cloud.model.operation.EntityOperation;
import yandex.cloud.model.operation.IdempotentOperation;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.repository.db.AbstractTable;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.task.DefaultTaskTable;
import yandex.cloud.task.TaskDb;
import yandex.cloud.task.TaskTable;
import yandex.cloud.task.model.Task;

public interface TeamIntegrationRepositoryTransaction extends RepositoryTransaction, OperationDb, TaskDb {

    default <T extends Entity<T>> AbstractTable<T> abstractTable(Class<T> entity) {
        return (AbstractTable<T>) table(entity);
    }


    @Override
    default OperationTable operations() {
        return new OperationTableImpl(abstractTable(Operation.class));
    }

    @Override
    default IdempotentOperationTable idempotentOperations() {
        return new IdempotentOperationTableImpl(abstractTable(IdempotentOperation.class));
    }

    @Override
    default EntityOperationTable entityOperations() {
        return new EntityOperationTableImpl(abstractTable(EntityOperation.class));
    }


    @Override
    default TaskTable tasks() {
        return new DefaultTaskTable(abstractTable(Task.class));
    }

}
