package ru.yandex.monlib.metrics.registry;

import java.time.Duration;

import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.meter.ExpMovingAverage;
import ru.yandex.monlib.metrics.meter.MaxMeter;
import ru.yandex.monlib.metrics.meter.Meter;

/**
 * @author Vladimir Gordiychuk
 */
public interface MeterFactory extends BaseFactory {
    default Meter oneMinuteMeter(String name) {
        return oneMinuteMeter(name, Labels.empty());
    }

    default Meter oneMinuteMeter(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        Meter meter = (Meter) getMetric(metricId);
        if (meter != null) {
            return meter;
        }
        return (Meter) putMetricIfAbsent(metricId, Meter.of(ExpMovingAverage.oneMinute()));
    }

    default Meter fiveMinutesMeter(String name) {
        return fiveMinutesMeter(name, Labels.empty());
    }

    default Meter fiveMinutesMeter(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        Meter meter = (Meter) getMetric(metricId);
        if (meter != null) {
            return meter;
        }
        return (Meter) putMetricIfAbsent(metricId, Meter.of(ExpMovingAverage.fiveMinutes()));
    }

    default Meter fifteenMinutesMeter(String name) {
        return fifteenMinutesMeter(name, Labels.empty());
    }

    default Meter fifteenMinutesMeter(String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        Meter meter = (Meter) getMetric(metricId);
        if (meter != null) {
            return meter;
        }
        return (Meter) putMetricIfAbsent(metricId, Meter.of(ExpMovingAverage.fifteenMinutes()));
    }

    default MaxMeter maxMeter(Duration window, String name) {
        return maxMeter(window, name, Labels.empty());
    }

    default MaxMeter maxMeter(Duration window, String name, Labels labels) {
        final MetricId metricId = new MetricId(name, labels);
        MaxMeter meter = (MaxMeter) getMetric(metricId);
        if (meter != null) {
            return meter;
        }
        return (MaxMeter) putMetricIfAbsent(metricId, new MaxMeter(0, window));
    }
}
