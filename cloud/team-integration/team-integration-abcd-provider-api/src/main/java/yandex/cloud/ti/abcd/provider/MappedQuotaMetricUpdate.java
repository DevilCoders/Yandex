package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record MappedQuotaMetricUpdate(
        @NotNull MappedAbcdResource abcdResource,
        long provided
) {
}
