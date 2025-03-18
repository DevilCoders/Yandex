package ru.yandex.monlib.metrics.summary;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;
import java.util.concurrent.atomic.DoubleAdder;

/**
 * @author Vladimir Gordiychuk
 */
public class SummaryDoubleCollectorImpl implements SummaryDoubleCollector {
    private static final AtomicLongFieldUpdater<SummaryDoubleCollectorImpl> countField =
            AtomicLongFieldUpdater.newUpdater(SummaryDoubleCollectorImpl.class, "count");
    private static final AtomicLongFieldUpdater<SummaryDoubleCollectorImpl> maxField =
            AtomicLongFieldUpdater.newUpdater(SummaryDoubleCollectorImpl.class, "max");
    private static final AtomicLongFieldUpdater<SummaryDoubleCollectorImpl> minField =
            AtomicLongFieldUpdater.newUpdater(SummaryDoubleCollectorImpl.class, "min");
    private static final AtomicLongFieldUpdater<SummaryDoubleCollectorImpl> lastField =
        AtomicLongFieldUpdater.newUpdater(SummaryDoubleCollectorImpl.class, "last");

    private final DoubleAdder sum = new DoubleAdder();
    private volatile long count;
    private volatile long min = Double.doubleToLongBits(Double.POSITIVE_INFINITY);
    private volatile long max = Double.doubleToLongBits(Double.NEGATIVE_INFINITY);
    private volatile long last;

    @Override
    public SummaryDoubleCollector collect(double value) {
        countField.incrementAndGet(this);
        sum.add(value);
        long raw = Double.doubleToRawLongBits(value);
        updateMax(value, raw);
        updateMin(value, raw);
        lastField.set(this, raw);
        return this;
    }

    private void updateMax(double value, long raw) {
        long prev;
        do {
            prev = max;
            double prevMax = Double.longBitsToDouble(prev);
            if (Double.compare(prevMax, value) >= 0) {
                return;
            }
        } while (!maxField.compareAndSet(this, prev, raw));
    }

    private void updateMin(double value, long raw) {
        long prev;
        do {
            prev = min;
            double prevMin = Double.longBitsToDouble(prev);
            if (Double.compare(prevMin, value) <= 0) {
                return;
            }
        } while (!minField.compareAndSet(this, prev, raw));
    }

    @Override
    public SummaryDoubleSnapshot snapshot() {
        return new ImmutableSummaryDoubleSnapshot(
            count,
            sum.sum(),
            Double.longBitsToDouble(min),
            Double.longBitsToDouble(max),
            Double.longBitsToDouble(last));
    }
}
