package yandex.cloud.ti.abc;

import org.jetbrains.annotations.NotNull;

public record AbcServiceCloud(
        long abcServiceId,
        @NotNull String abcServiceSlug,
        @NotNull String abcdFolderId,
        @NotNull String cloudId,
        @NotNull String defaultFolderId
) {
}
