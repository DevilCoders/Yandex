package ru.yandex.monlib.metrics.primitives;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class GaugeInt64 implements Metric {
    private static final AtomicLongFieldUpdater<GaugeInt64> valueField =
            AtomicLongFieldUpdater.newUpdater(GaugeInt64.class, "value");

    private volatile long value;

    public GaugeInt64() {
        this(0);
    }

    public GaugeInt64(long value) {
        this.value = value;
    }

    public long get() {
        return value;
    }

    public void set(long value) {
        this.value = value;
    }

    public void add(long value) {
        valueField.addAndGet(this, value);
    }

    public void combine(GaugeInt64 gauge) {
        add(gauge.value);
    }

    @Override
    public MetricType type() {
        return MetricType.IGAUGE;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, value);
    }

    @Override
    public String toString() {
        return "GaugeInt64{" + value + '}';
    }
}
