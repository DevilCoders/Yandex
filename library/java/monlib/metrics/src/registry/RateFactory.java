package ru.yandex.monlib.metrics.registry;

import java.util.function.LongSupplier;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.LazyRate;
import ru.yandex.monlib.metrics.primitives.Rate;


/**
 * @author Sergey Polovko
 */
interface RateFactory extends BaseFactory {

    default Rate rate(String name) {
        return rate(name, Labels.empty());
    }

    default Rate rate(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        Rate rate = (Rate) getMetric(metricId);
        if (rate != null) {
            return rate;
        }
        return (Rate) putMetricIfAbsent(metricId, new Rate());
    }

    default LazyRate lazyRate(String name, LongSupplier supplier) {
        return lazyRate(name, Labels.empty(), supplier);
    }

    default LazyRate lazyRate(String name, Labels labels, LongSupplier supplier) {
        final MetricId metricId = new MetricId(name, labels);
        LazyRate rate = (LazyRate) getMetric(metricId);
        if (rate != null) {
            return rate;
        }
        return (LazyRate) putMetricIfAbsent(metricId, new LazyRate(supplier));
    }
}
