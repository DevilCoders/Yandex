package yandex.cloud.ti.abcd.provider;

import org.jetbrains.annotations.NotNull;

public record MappedQuotaMetricValue(
        @NotNull MappedAbcdResource abcdResource,
        long provided,
        long allocated
) {
}
