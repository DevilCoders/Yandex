package yandex.cloud.ti.rm.client;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public record Cloud(
        @NotNull String id,
        @Nullable String organizationId
) {
}
