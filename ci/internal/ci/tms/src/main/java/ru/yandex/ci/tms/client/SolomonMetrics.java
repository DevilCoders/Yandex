package ru.yandex.ci.tms.client;

import java.util.Collection;
import java.util.List;

import lombok.Builder;
import lombok.Singular;
import lombok.Value;

@Value
@Builder
public class SolomonMetrics {
    @Singular
    List<SolomonMetric> metrics;

    public static SolomonMetrics of(Collection<SolomonMetric> metrics) {
        return SolomonMetrics.builder()
                .metrics(metrics)
                .build();
    }
}
