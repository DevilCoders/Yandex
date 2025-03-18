package ru.yandex.monlib.metrics.example.services.series.metrics;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricSupplier;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * Counts episode views:
 * <p>
 * <p> COUNTER episode.view.count - episode view rate based on slug that matches pattern from
 * https://wiki.yandex-team.ru/solomon/userguide/metricsnamingconventions/#metki
 *
 * @author Alexey Trushkin
 */
public class EpisodeViewMetrics implements MetricSupplier {

    private final MetricRegistry registry;
    private final Rate count;

    public EpisodeViewMetrics(String slug) {
        // create new registry because it's example of MetricSupplier
        registry = new MetricRegistry(Labels.of("slug", slug));
        count = registry.rate("episode.view.count");
    }

    public long incrementViewCount() {
        return count.inc();
    }

    public void combine(EpisodeViewMetrics episodeViewMetrics) {
        count.combine(episodeViewMetrics.count);
    }

    @Override
    public int estimateCount() {
        return registry.estimateCount();
    }

    @Override
    public void append(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
        registry.append(tsMillis, commonLabels, consumer);
    }
}
