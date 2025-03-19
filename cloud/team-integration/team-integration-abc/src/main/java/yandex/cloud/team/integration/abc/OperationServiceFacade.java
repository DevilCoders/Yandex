package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;

public interface OperationServiceFacade {

    Operation getOperation(
            @NotNull String operationId
    );

}
