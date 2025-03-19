package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.NotFoundException;

public class AbcdProviderNotFoundException extends NotFoundException {

    public AbcdProviderNotFoundException(@NotNull String message) {
        super(message);
    }

}
