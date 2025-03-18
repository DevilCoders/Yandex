package ru.yandex.ci.tms.metric.ci;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import lombok.ToString;
import one.util.streamex.StreamEx;

import ru.yandex.ci.ydb.service.metric.MetricId;

@ToString
class FakeMetricsConsumer implements AbstractUsageMetricTask.MetricConsumer {

    private final Instant now;
    private final Map<String, Double> metrics = new LinkedHashMap<>();

    FakeMetricsConsumer(Instant now) {
        this.now = now;
    }

    FakeMetricsConsumer() {
        this(Instant.now());
    }

    @Override
    public void addMetric(MetricId metricId, double value) {
        metrics.put(metricId.asString(), value);
    }

    public List<MetricId> getMetricIds() {
        return StreamEx.ofKeys(metrics).map(MetricId::ofString).toList();
    }

    public Map<String, Double> getMetrics() {
        return metrics;
    }

    @Override
    public void addMetric(MetricId metricId, double value, Instant time) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Instant now() {
        return now;
    }
}
