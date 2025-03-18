package ru.yandex.monlib.metrics.summary;

import java.util.concurrent.atomic.AtomicLongFieldUpdater;

/**
 * @author Vladimir Gordiychuk
 */
public class SummaryInt64CollectorImpl implements SummaryInt64Collector {
    private static final AtomicLongFieldUpdater<SummaryInt64CollectorImpl> countField =
            AtomicLongFieldUpdater.newUpdater(SummaryInt64CollectorImpl.class, "count");
    private static final AtomicLongFieldUpdater<SummaryInt64CollectorImpl> sumField =
            AtomicLongFieldUpdater.newUpdater(SummaryInt64CollectorImpl.class, "sum");
    private static final AtomicLongFieldUpdater<SummaryInt64CollectorImpl> minField =
            AtomicLongFieldUpdater.newUpdater(SummaryInt64CollectorImpl.class, "min");
    private static final AtomicLongFieldUpdater<SummaryInt64CollectorImpl> maxField =
            AtomicLongFieldUpdater.newUpdater(SummaryInt64CollectorImpl.class, "max");
    private static final AtomicLongFieldUpdater<SummaryInt64CollectorImpl> lastField =
        AtomicLongFieldUpdater.newUpdater(SummaryInt64CollectorImpl.class, "last");

    private volatile long count;
    private volatile long sum;
    private volatile long min = Long.MAX_VALUE;
    private volatile long max = Long.MIN_VALUE;
    private volatile long last;

    @Override
    public SummaryInt64Collector collect(long value) {
        countField.incrementAndGet(this);
        sumField.addAndGet(this, value);
        updateMax(value);
        updateMin(value);
        lastField.set(this, value);
        return this;
    }

    private void updateMax(long next) {
        long prev;
        do {
            prev = max;
            if (prev >= next) {
                return;
            }
        } while (!maxField.compareAndSet(this, prev, next));
    }

    private void updateMin(long next) {
        long prev;
        do {
            prev = min;
            if (min <= next) {
                return;
            }
        } while (!minField.compareAndSet(this, prev, next));
    }

    @Override
    public SummaryInt64Snapshot snapshot() {
        return new ImmutableSummaryInt64Snapshot(count, sum, min, max, last);
    }
}
