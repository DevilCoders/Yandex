package yandex.cloud.ti.operation;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.audit.OperationDb;
import yandex.cloud.model.operation.Operation;

class OperationServiceImpl implements OperationService {

    private final @NotNull OperationDb operationDb;


    OperationServiceImpl(
            @NotNull OperationDb operationDb
    ) {
        this.operationDb = operationDb;
    }


    @Override
    public @NotNull Operation saveOperation(
            @NotNull Operation operation
    ) {
        return operationDb.operations()
                .save(operation);
    }

    @Override
    public @Nullable Operation findOperation(
            @NotNull Operation.Id operationId
    ) {
        return operationDb.operations()
                .find(operationId);
    }

}
