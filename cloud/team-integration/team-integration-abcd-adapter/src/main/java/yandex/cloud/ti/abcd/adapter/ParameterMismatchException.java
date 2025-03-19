package yandex.cloud.ti.abcd.adapter;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.FailedPreconditionException;

/**
 * Represents an error condition occurred for a mismatched parameter.
 */
public class ParameterMismatchException extends FailedPreconditionException {

    @Serial
    private static final long serialVersionUID = 3697185978265509124L;

    private ParameterMismatchException(String message) {
        super(message);
    }

    public static @NotNull ParameterMismatchException of(
            long expected,
            long actual,
            @NotNull String parameterName
    ) {
        return new ParameterMismatchException(
                String.format("Parameter '%s' is invalid. Expected %d, got %d",
                        parameterName, expected, actual));
    }

    public static @NotNull ParameterMismatchException of(
            /* nullable */ Object expected,
            /* nullable */ Object actual,
            @NotNull String parameterName
    ) {
        return new ParameterMismatchException(
                String.format("Parameter '%s' is invalid. Expected '%s', got '%s'",
                        parameterName, expected, actual));
    }

}
