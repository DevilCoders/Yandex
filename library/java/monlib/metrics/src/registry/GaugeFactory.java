package ru.yandex.monlib.metrics.registry;

import java.util.function.DoubleSupplier;
import java.util.function.LongSupplier;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.GaugeDouble;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.LazyGaugeDouble;
import ru.yandex.monlib.metrics.primitives.LazyGaugeInt64;


/**
 * @author Sergey Polovko
 */
interface GaugeFactory extends BaseFactory {

    // -- Double ----

    default GaugeDouble gaugeDouble(String name) {
        return gaugeDouble(name, Labels.empty());
    }

    default GaugeDouble gaugeDouble(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        GaugeDouble gauge = (GaugeDouble) getMetric(metricId);
        if (gauge != null) {
            return gauge;
        }
        return (GaugeDouble) putMetricIfAbsent(metricId, new GaugeDouble());
    }

    default LazyGaugeDouble lazyGaugeDouble(String name, DoubleSupplier supplier) {
        return lazyGaugeDouble(name, Labels.empty(), supplier);
    }

    default LazyGaugeDouble lazyGaugeDouble(String name, Labels labels, DoubleSupplier supplier) {
        final MetricId metricId = new MetricId(name, labels);
        LazyGaugeDouble gauge = (LazyGaugeDouble) getMetric(metricId);
        if (gauge != null) {
            return gauge;
        }
        return (LazyGaugeDouble) putMetricIfAbsent(metricId, new LazyGaugeDouble(supplier));
    }

    // -- Int64 ----

    default GaugeInt64 gaugeInt64(String name) {
        return gaugeInt64(name, Labels.empty());
    }

    default GaugeInt64 gaugeInt64(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        GaugeInt64 gauge = (GaugeInt64) getMetric(metricId);
        if (gauge != null) {
            return gauge;
        }
        return (GaugeInt64) putMetricIfAbsent(metricId, new GaugeInt64());
    }

    default LazyGaugeInt64 lazyGaugeInt64(String name, LongSupplier supplier) {
        return lazyGaugeInt64(name, Labels.empty(), supplier);
    }

    default LazyGaugeInt64 lazyGaugeInt64(String name, Labels labels, LongSupplier supplier) {
        final MetricId metricId = new MetricId(name, labels);
        LazyGaugeInt64 gauge = (LazyGaugeInt64) getMetric(metricId);
        if (gauge != null) {
            return gauge;
        }
        return (LazyGaugeInt64) putMetricIfAbsent(metricId, new LazyGaugeInt64(supplier));
    }
}
