package yandex.cloud.ti.abcd.provider;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public record AbcdResourceKey(
        @NotNull String resourceTypeKey,
        @NotNull List<AbcdResourceSegmentKey> resourceSegmentKeys
) {

    public AbcdResourceKey(
            @NotNull String resourceTypeKey,
            @Nullable List<AbcdResourceSegmentKey> resourceSegmentKeys
    ) {
        this.resourceTypeKey = resourceTypeKey;
        this.resourceSegmentKeys = resourceSegmentKeys == null ? List.of() : List.copyOf(resourceSegmentKeys);
    }

}
