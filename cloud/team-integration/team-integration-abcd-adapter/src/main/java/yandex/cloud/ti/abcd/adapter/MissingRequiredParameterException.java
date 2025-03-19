package yandex.cloud.ti.abcd.adapter;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.BadRequestException;

/**
 * Represents an error condition occurred for a missing parameter.
 */
public class MissingRequiredParameterException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 3697185978265509124L;

    private MissingRequiredParameterException(String message) {
        super(message);
    }

    public static @NotNull MissingRequiredParameterException of(
            @NotNull String parameterName
    ) {
        return new MissingRequiredParameterException(
                String.format("Required parameter '%s' is not set", parameterName));
    }

}
