package yandex.cloud.ti.rm.client;

import org.jetbrains.annotations.NotNull;

public record Folder(
        @NotNull String id,
        @NotNull String cloudId
) {
}
