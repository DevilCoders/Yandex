package ru.yandex.monlib.metrics.meter;

import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;


/**
 * @author Sergey Polovko
 */
public final class Meter extends TickMixin implements Metric {
    private final ExpMovingAverage movingAverage;

    private Meter(ExpMovingAverage movingAverage) {
        super(movingAverage.getTickIntervalNanos());
        this.movingAverage = movingAverage;
    }

    public static Meter of(ExpMovingAverage movingAverage) {
        return new Meter(movingAverage);
    }

    @Override
    protected void onTick() {
        movingAverage.tick();
    }

    public void mark() {
        tickIfNecessary();
        movingAverage.inc();
    }

    public void mark(long n) {
        tickIfNecessary();
        movingAverage.update(n);
    }

    public double getRate(TimeUnit unit) {
        tickIfNecessary();
        return movingAverage.getRate(unit);
    }

    @Override
    public MetricType type() {
        return MetricType.DGAUGE;
    }

    @Override
    public void accept(long tsMillis, MetricConsumer consumer) {
        consumer.onDouble(tsMillis, movingAverage.getRate(TimeUnit.SECONDS));
    }

    public void combine(Meter meter) {
        movingAverage.combine(meter.movingAverage);
    }

    @Override
    public String toString() {
        return "Meter{" + movingAverage.getRate(TimeUnit.SECONDS) + "/s}";
    }
}
