package ru.yandex.monlib.metrics.primitives;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class Counter implements Metric {

    private static final AtomicLongFieldUpdater<Counter> valueUpdater =
        AtomicLongFieldUpdater.newUpdater(Counter.class, "value");

    private volatile long value;

    public Counter() {
        this(0);
    }

    public Counter(long value) {
        this.value = value;
    }

    public long inc() {
        return valueUpdater.incrementAndGet(this);
    }

    public long add(long n) {
        return valueUpdater.addAndGet(this, n);
    }

    public long get() {
        return valueUpdater.get(this);
    }

    public void reset() {
        valueUpdater.set(this, 0);
    }

    public void combine(Counter counter) {
        valueUpdater.addAndGet(this, counter.get());
    }

    @Override
    public MetricType type() {
        return MetricType.COUNTER;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, value);
    }

    @Override
    public String toString() {
        return "Counter{" + value + '}';
    }
}
