package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record MappedAbcdResource(
        @NotNull AbcdResourceKey resourceKey,
        @NotNull String unitKey
) {
}
