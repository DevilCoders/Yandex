package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record MappedQuotaMetric(
        @NotNull String name,
        @NotNull MappedAbcdResource abcdResource
) {
}
