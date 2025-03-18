package ru.yandex.monlib.metrics.registry;

import java.util.function.LongSupplier;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Counter;
import ru.yandex.monlib.metrics.primitives.LazyCounter;


/**
 * @author Sergey Polovko
 */
@ParametersAreNonnullByDefault
interface CounterFactory extends BaseFactory {

    default Counter counter(String name) {
        return counter(name, Labels.empty());
    }

    default Counter counter(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        Counter counter = (Counter) getMetric(metricId);
        if (counter != null) {
            return counter;
        }
        return (Counter) putMetricIfAbsent(metricId, new Counter());
    }

    default LazyCounter lazyCounter(String name, LongSupplier supplier) {
        return lazyCounter(name, Labels.empty(), supplier);
    }

    default LazyCounter lazyCounter(String name, Labels labels, LongSupplier supplier) {
        final MetricId metricId = new MetricId(name, labels);
        LazyCounter counter = (LazyCounter) getMetric(metricId);
        if (counter != null) {
            return counter;
        }
        return (LazyCounter) putMetricIfAbsent(metricId, new LazyCounter(supplier));
    }
}
