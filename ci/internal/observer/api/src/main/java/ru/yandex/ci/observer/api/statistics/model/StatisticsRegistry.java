package ru.yandex.ci.observer.api.statistics.model;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import lombok.Value;

@Value
public class StatisticsRegistry {
    MeterRegistry meter;
    CollectorRegistry registry;
}
