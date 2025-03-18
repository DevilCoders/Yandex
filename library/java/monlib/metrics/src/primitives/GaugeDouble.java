package ru.yandex.monlib.metrics.primitives;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class GaugeDouble implements Metric {
    private static final AtomicLongFieldUpdater<GaugeDouble> rawValueField =
            AtomicLongFieldUpdater.newUpdater(GaugeDouble.class, "longBits");

    private volatile long longBits;

    public GaugeDouble() {
        this(0.0);
    }

    public GaugeDouble(double value) {
        this.longBits = Double.doubleToLongBits(value);
    }

    public double get() {
        return Double.longBitsToDouble(longBits);
    }

    public void set(double value) {
        rawValueField.set(this, Double.doubleToLongBits(value));
    }

    public void add(double value) {
        long prev;
        long next;
        do {
            prev = this.longBits;
            double prevValue = Double.longBitsToDouble(prev);
            double sum = prevValue + value;
            next = Double.doubleToLongBits(sum);
        } while (!rawValueField.compareAndSet(this, prev, next));
    }

    public void combine(GaugeDouble gauge) {
        add(gauge.get());
    }

    @Override
    public MetricType type() {
        return MetricType.DGAUGE;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onDouble(tsMillis, get());
    }

    @Override
    public String toString() {
        return "GaugeDouble{" + get() + '}';
    }
}
