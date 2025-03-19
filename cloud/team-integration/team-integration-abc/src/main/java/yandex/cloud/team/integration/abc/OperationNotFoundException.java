package yandex.cloud.team.integration.abc;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.NotFoundException;

/**
 * Represents an client error condition for an operation that is not found (404 for HTTP).
 */
public class OperationNotFoundException extends NotFoundException {

    @Serial
    private static final long serialVersionUID = 486457367785153729L;

    private OperationNotFoundException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull OperationNotFoundException of(
            @NotNull String operationId
    ) {
        return new OperationNotFoundException(
                String.format("Operation with id '%s' not found", operationId));
    }

}
