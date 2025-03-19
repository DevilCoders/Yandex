package yandex.cloud.ti.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;

public record AbcServiceCloudStubOperationReference(
        @NotNull Operation.Id stubOperationId,
        @NotNull Operation.Id createOperationId
) {
}
