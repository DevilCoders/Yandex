package ru.yandex.ci.ydb.service.metric;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.util.concurrent.AbstractScheduledService;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.db.CiMainDb;

/**
 * Сервис для отдачи метрик из YDB в соломон с кешированием.
 * Нужен для метрик, пересчет которых дорогой и редкий,
 * что бы можно было считать их периодически в TMS тасках и отдавать их
 */
@Slf4j
public class YdbSolomonMetricService extends AbstractScheduledService {

    private final CiMainDb db;
    private final Duration metricUpdateInterval;
    private final Duration metricTtl;
    private final List<MetricId> metricIds;

    private final ConcurrentHashMap<MetricId, Metric> metrics = new ConcurrentHashMap<>();

    public YdbSolomonMetricService(MeterRegistry meterRegistry,
                                   CiMainDb db,
                                   Duration metricUpdateInterval,
                                   Duration metricTtl,
                                   List<MetricId> metricIds) {
        this.db = db;
        this.metricUpdateInterval = metricUpdateInterval;
        this.metricTtl = metricTtl;
        this.metricIds = metricIds;
        registerMetrics(meterRegistry, metricIds);
    }

    private void registerMetrics(MeterRegistry meterRegistry, List<MetricId> metricIds) {
        for (MetricId metricId : metricIds) {
            Gauge.builder(metricId.getName(), () -> getValue(metricId))
                    .tags(metricId.getTags())
                    .register(meterRegistry);
        }
    }

    @Override
    protected void runOneIteration() {
        try {
            db.currentOrReadOnly(this::updateMetrics);
        } catch (Exception e) {
            log.error("failed to updateMetrics", e);
        }
    }

    private void updateMetrics() {
        log.info("Updating {} metrics", metricIds.size());
        for (MetricId metricId : metricIds) {
            db.metrics().getLastMetric(metricId)
                    .ifPresent(metric -> metrics.put(metricId, metric));
        }
        log.info("Updated {} metrics", metricIds.size());
    }

    @VisibleForTesting
    protected Instant now() {
        return Instant.now();
    }

    @Override
    protected Scheduler scheduler() {
        return Scheduler.newFixedDelaySchedule(Duration.ZERO, metricUpdateInterval);
    }

    private double getValue(MetricId metricId) {
        Metric metric = metrics.get(metricId);
        if (metric == null) {
            return Double.NaN;
        }
        if (metric.getTime().plus(metricTtl).isBefore(now())) {
            return Double.NaN;
        }
        return metric.getValue();
    }

}
