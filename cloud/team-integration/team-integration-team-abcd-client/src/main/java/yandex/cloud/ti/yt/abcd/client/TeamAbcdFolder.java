package yandex.cloud.ti.yt.abcd.client;

import org.jetbrains.annotations.NotNull;

public record TeamAbcdFolder(
        @NotNull String id,
        long abcServiceId,
        @NotNull String name,
        boolean defaultForService
) {
}
