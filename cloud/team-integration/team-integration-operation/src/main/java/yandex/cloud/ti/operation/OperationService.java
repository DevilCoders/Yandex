package yandex.cloud.ti.operation;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.model.operation.Operation;

public interface OperationService {

    @NotNull Operation saveOperation(
            @NotNull Operation operation
    );

    @Nullable Operation findOperation(
            @NotNull Operation.Id operationId
    );

}
