package ru.yandex.monlib.metrics.primitives;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.histogram.HistogramCollector;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;


/**
 * @author Sergey Polovko
 */
public final class Histogram implements Metric {

    private final boolean isRate;
    private final HistogramCollector collector;

    private Histogram(boolean isRate, HistogramCollector collector) {
        this.isRate = isRate;
        this.collector = collector;
    }

    public static Histogram newCounter(HistogramCollector collector) {
        return new Histogram(false, collector);
    }

    public static Histogram newRate(HistogramCollector collector) {
        return new Histogram(true, collector);
    }

    public void record(long value) {
        collector.collect(value);
    }

    public void record(long value, long count) {
        collector.collect(value, count);
    }

    public void record(HistogramSnapshot snapshot) {
        collector.collect(snapshot);
    }

    public void combine(Histogram histogram) {
        collector.combine(histogram.collector);
    }

    public HistogramSnapshot snapshot() {
        return collector.snapshot();
    }

    @Override
    public MetricType type() {
        return isRate ? MetricType.HIST_RATE : MetricType.HIST;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onHistogram(tsMillis, collector.snapshot());
    }

    @Override
    public String toString() {
        return "Histogram{isRate=" + isRate + ", collector=" + collector + '}';
    }
}
