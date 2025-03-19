package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.AlreadyExistsException;

public class AbcdAccountAlreadyExistsException extends AlreadyExistsException {

    public AbcdAccountAlreadyExistsException(
            @NotNull String message
    ) {
        super(message);
    }

}
