package yandex.cloud.ti.yt.abc.client;

import org.jetbrains.annotations.NotNull;

public record TeamAbcService(
        long id,
        @NotNull String slug,
        @NotNull String name
) {
}
