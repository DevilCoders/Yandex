package ru.yandex.monlib.metrics.registry;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.monlib.metrics.Metric;


/**
 * @author Sergey Polovko
 */
interface BaseFactory {

    @Nullable
    Metric getMetric(MetricId id);

    @Nonnull
    Metric putMetricIfAbsent(MetricId id, Metric metric);
}
