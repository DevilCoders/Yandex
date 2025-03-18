package ru.yandex.monlib.metrics.meter;

import java.time.Duration;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;

/**
 * This meter uses two adjacent windows of same size.
 * Values are accepted to the last one. The meter value is maximum
 * over the both windows. Thus the meter value is meaningful when
 * the last window is not populated yet. If some big value was added to
 * the meter it would be evicted after two window intervals at most
 *
 * @author Ivan Tsybulin
 */
@ParametersAreNonnullByDefault
public final class MaxMeter extends TickMixin implements Metric {
    private final TumblingMax tumblingMax;

    public MaxMeter(long minValue, Duration windowSize) {
        super(windowSize.toNanos());
        tumblingMax = new TumblingMax(minValue);
    }

    public void record(long value) {
        tickIfNecessary();
        tumblingMax.accept(value);
    }

    public long getMax() {
        tickIfNecessary();
        return tumblingMax.getMax();
    }

    @Override
    protected void onTick() {
        tumblingMax.tick();
    }

    @Override
    public MetricType type() {
        return MetricType.IGAUGE;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onLong(tsMillis, getMax());
    }
}
