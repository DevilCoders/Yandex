package ru.yandex.monlib.metrics.primitives;

import java.util.function.DoubleSupplier;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public class LazyGaugeDouble implements Metric {

    private final DoubleSupplier supplier;

    public LazyGaugeDouble(DoubleSupplier supplier) {
        this.supplier = supplier;
    }

    @Override
    public MetricType type() {
        return MetricType.DGAUGE;
    }

    public double get() {
        return supplier.getAsDouble();
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onDouble(tsMillis, get());
    }

    @Override
    public String toString() {
        return "LazyGaugeDouble{" + supplier.getAsDouble() + '}';
    }
}
