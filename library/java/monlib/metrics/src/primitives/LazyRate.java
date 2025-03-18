package ru.yandex.monlib.metrics.primitives;

import java.util.function.LongSupplier;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class LazyRate implements Metric {

    private final LongSupplier supplier;

    public LazyRate(LongSupplier supplier) {
        this.supplier = supplier;
    }

    @Override
    public MetricType type() {
        return MetricType.RATE;
    }

    public long get() {
        return supplier.getAsLong();
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, get());
    }

    @Override
    public String toString() {
        return "LazyRate{" + supplier.getAsLong() + '}';
    }
}
