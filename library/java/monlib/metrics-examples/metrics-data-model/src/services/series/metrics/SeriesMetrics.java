package ru.yandex.monlib.metrics.example.services.series.metrics;

import java.util.concurrent.ConcurrentHashMap;
import java.util.function.LongSupplier;

import org.springframework.stereotype.Component;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricSupplier;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * Advanced example with MetricSupplier and aggregation
 * Series metrics:
 * <p>
 * <p> IGAUGE series.count - current series count
 *
 * @author Alexey Trushkin
 */
@Component
public class SeriesMetrics implements MetricSupplier {

    private final ConcurrentHashMap<Long, EpisodeViewMetrics> episodeViews = new ConcurrentHashMap<>();

    private final MetricRegistry registry;
    private final GaugeInt64 seriesCount;

    public SeriesMetrics() {
        // create new registry for MetricSupplier
        registry = new MetricRegistry();
        // isn't lazy just for eager metric example
        seriesCount = registry.gaugeInt64("series.count", Labels.of());
    }

    @Override
    public int estimateCount() {
        return episodeViews.size() + 1 + registry.estimateCount();
    }

    @Override
    public void append(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
        // aggregate episode view metric to total metric
        EpisodeViewMetrics total = new EpisodeViewMetrics("total");
        for (EpisodeViewMetrics episodeViewMetrics : episodeViews.values()) {
            // add episode metric
            episodeViewMetrics.append(tsMillis, commonLabels, consumer);
            // aggregate sum
            total.combine(episodeViewMetrics);
        }
        // add episode total metric
        total.append(tsMillis, commonLabels, consumer);

        // add series summary metrics
        registry.append(tsMillis, commonLabels, consumer);
    }

    public Long incrementViewCount(long id, String slug) {
        var metric = episodeViews.computeIfAbsent(id, key -> new EpisodeViewMetrics(slug));
        return metric.incrementViewCount();
    }

    public void decrementSeriesCount() {
        seriesCount.add(-1);
    }

    public void incrementSeriesCount() {
        seriesCount.add(1);
    }

    public void removeEpisodeMetrics(long episodeId) {
        episodeViews.remove(episodeId);
    }

    public void registerType(String type, LongSupplier supplier) {
        registry.lazyGaugeInt64(type + ".count", Labels.of(), supplier);
    }
}
