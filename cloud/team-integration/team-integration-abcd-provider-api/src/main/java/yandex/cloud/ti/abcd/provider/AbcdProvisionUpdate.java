package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record AbcdProvisionUpdate(
        @NotNull AbcdResourceKey abcdResourceKey,
        long provided,
        @NotNull String abcdUnitKey
) {
}
