package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.NotFoundException;

public class AbcdAccountNotFoundException extends NotFoundException {

    public AbcdAccountNotFoundException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull AbcdAccountNotFoundException of(
            @NotNull String accountId
    ) {
        return new AbcdAccountNotFoundException(
                String.format("Account '%s' not found", accountId));
    }

}
