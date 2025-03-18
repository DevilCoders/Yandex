package ru.yandex.monlib.metrics.primitives;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class Rate implements Metric {

    private static final AtomicLongFieldUpdater<Rate> valueUpdater =
        AtomicLongFieldUpdater.newUpdater(Rate.class, "value");

    private volatile long value;

    public Rate() {
        this(0);
    }

    public Rate(long value) {
        this.value = value;
    }

    public long inc() {
        return valueUpdater.incrementAndGet(this);
    }

    public long add(long n) {
        return valueUpdater.addAndGet(this, n);
    }

    public void set(long n) {
        valueUpdater.set(this, n);
    }

    public long get() {
        return valueUpdater.get(this);
    }

    public void combine(Rate rate) {
        valueUpdater.addAndGet(this, rate.value);
    }

    @Override
    public MetricType type() {
        return MetricType.RATE;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, value);
    }

    @Override
    public String toString() {
        return "Rate{" + value + '}';
    }
}
