package yandex.cloud.ti.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.model.operation.Operation;

public record AbcServiceCloudCreateOperationReference(
        long abcServiceId,
        @NotNull Operation.Id createOperationId
) {
}
