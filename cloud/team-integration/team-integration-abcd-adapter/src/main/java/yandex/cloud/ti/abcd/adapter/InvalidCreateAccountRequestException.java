package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.iam.exception.BadRequestException;

public class InvalidCreateAccountRequestException extends BadRequestException {

    public InvalidCreateAccountRequestException(String message) {
        super(message);
    }

    public InvalidCreateAccountRequestException(String message, String internal) {
        super(message, internal);
    }


    public static @NotNull InvalidCreateAccountRequestException parameterMismatch(
            @Nullable String operationId,
            @NotNull String parameterName,
            @Nullable Object operationValue, @Nullable Object requestValue
    ) {
        return new InvalidCreateAccountRequestException(
                "requests with the same operation_id must have the same set of parameters",
                String.format("operation_id=%s, parameter=%s, expected=%s, actual=%s",
                        operationId,
                        parameterName,
                        operationValue, requestValue
                )
        );
    }

}
