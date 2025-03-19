package yandex.cloud.ti.abcd.provider;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public record AbcdProviderProperties(
        @NotNull String id,
        @NotNull String name,
        @NotNull CloudQuotaServiceProperties quotaService,
        int quotaQueryThreads,
        @NotNull List<MappedQuotaMetric> mappedQuotaMetrics
) {

    public AbcdProviderProperties(
            @NotNull String id,
            @NotNull String name,
            @NotNull CloudQuotaServiceProperties quotaService,
            int quotaQueryThreads,
            @Nullable List<MappedQuotaMetric> mappedQuotaMetrics
    ) {
        this.id = id;
        this.name = name;
        this.quotaService = quotaService;
        if (quotaQueryThreads < 0 || quotaQueryThreads > 10) {
            throw new IllegalArgumentException("quotaQueryThreads should be in [0..10] range, got %d".formatted(quotaQueryThreads));
        }
        this.quotaQueryThreads = Math.max(quotaQueryThreads, 1);
        this.mappedQuotaMetrics = mappedQuotaMetrics == null ? List.of() : List.copyOf(mappedQuotaMetrics);
    }

}
