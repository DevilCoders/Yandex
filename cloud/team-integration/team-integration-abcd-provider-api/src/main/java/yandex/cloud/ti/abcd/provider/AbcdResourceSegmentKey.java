package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record AbcdResourceSegmentKey(
        @NotNull String segmentationKey,
        @NotNull String segmentKey
) {
}
