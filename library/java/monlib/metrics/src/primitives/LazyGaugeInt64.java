package ru.yandex.monlib.metrics.primitives;

import java.util.function.LongSupplier;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class LazyGaugeInt64 implements Metric {

    private final LongSupplier supplier;

    public LazyGaugeInt64(LongSupplier supplier) {
        this.supplier = supplier;
    }

    @Override
    public MetricType type() {
        return MetricType.IGAUGE;
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
        return "LazyGaugeInt64{" + supplier.getAsLong() + '}';
    }
}
