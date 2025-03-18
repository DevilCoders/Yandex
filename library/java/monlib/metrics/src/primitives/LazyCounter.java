package ru.yandex.monlib.metrics.primitives;

import java.util.function.LongSupplier;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;

/**
 * @author Vladimir Gordiychuk
 */
public class LazyCounter implements Metric {
    private final LongSupplier supplier;

    public LazyCounter(LongSupplier supplier) {
        this.supplier = supplier;
    }

    @Override
    public MetricType type() {
        return MetricType.COUNTER;
    }

    public long get() {
        return supplier.getAsLong();
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, supplier.getAsLong());
    }

    @Override
    public String toString() {
        return "LazyCounter{" + supplier.getAsLong() + '}';
    }
}
