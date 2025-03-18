package ru.yandex.monlib.metrics.registry;

import java.util.function.Supplier;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Histogram;


/**
 * @author Sergey Polovko
 */
@ParametersAreNonnullByDefault
interface HistogramFactory extends BaseFactory {

    default Histogram histogramCounter(String name, HistogramCollector collector) {
        return histogramCounter(name, Labels.empty(), collector);
    }

    default Histogram histogramCounter(String name, Labels labels, HistogramCollector collector) {
        final MetricId metricId = new MetricId(name, labels);
        Histogram histogram = (Histogram) getMetric(metricId);
        if (histogram != null) {
            return histogram;
        }
        return (Histogram) putMetricIfAbsent(metricId, Histogram.newCounter(collector));
    }

    default Histogram histogramRate(String name, HistogramCollector collector) {
        return histogramRate(name, Labels.empty(), collector);
    }

    default Histogram histogramRate(String name, Labels labels, HistogramCollector collector) {
        final MetricId metricId = new MetricId(name, labels);
        Histogram histogram = (Histogram) getMetric(metricId);
        if (histogram != null) {
            return histogram;
        }
        return (Histogram) putMetricIfAbsent(metricId, Histogram.newRate(collector));
    }

    default Histogram histogramCounter(String name, Supplier<HistogramCollector> collectorSupplier) {
        return histogramCounter(name, Labels.empty(), collectorSupplier);
    }

    default Histogram histogramCounter(String name, Labels labels, Supplier<HistogramCollector> collectorSupplier) {
        final MetricId metricId = new MetricId(name, labels);
        Histogram histogram = (Histogram) getMetric(metricId);
        if (histogram != null) {
            return histogram;
        }
        return (Histogram) putMetricIfAbsent(metricId, Histogram.newCounter(collectorSupplier.get()));
    }

    default Histogram histogramRate(String name, Supplier<HistogramCollector> collectorSupplier) {
        return histogramRate(name, Labels.empty(), collectorSupplier);
    }

    default Histogram histogramRate(String name, Labels labels, Supplier<HistogramCollector> collectorSupplier) {
        final MetricId metricId = new MetricId(name, labels);
        Histogram histogram = (Histogram) getMetric(metricId);
        if (histogram != null) {
            return histogram;
        }
        return (Histogram) putMetricIfAbsent(metricId, Histogram.newRate(collectorSupplier.get()));
    }

}
