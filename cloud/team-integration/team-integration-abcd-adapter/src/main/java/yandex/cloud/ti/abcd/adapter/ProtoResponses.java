package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

final class ProtoResponses {

    static @NotNull String optionalField(@Nullable String value) {
        if (value == null) {
            return "";
        }
        return value;
    }


    private ProtoResponses() {
    }

}
