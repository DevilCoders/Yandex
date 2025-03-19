package yandex.cloud.ti.abcd.adapter;

import org.jetbrains.annotations.NotNull;

public final class TestAbcdAdapters {

    public static @NotNull String templatePassportUid(
            long userId
    ) {
        return "112%013d".formatted(userId);
    }

    public static @NotNull String templateStaffLogin(
            long userId
    ) {
        return "user%d".formatted(userId);
    }


    private TestAbcdAdapters() {
    }

}
