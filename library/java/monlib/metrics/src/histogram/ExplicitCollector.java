package ru.yandex.monlib.metrics.histogram;

import java.util.concurrent.atomic.AtomicLongArray;


/**
 * @author Sergey Polovko
 */
final class ExplicitCollector implements HistogramCollector {
    private final double[] bounds;
    private final AtomicLongArray buckets;

    ExplicitCollector(double[] bounds) {
        // copy given bounds and add one bucket as +INF
        this.bounds = new double[bounds.length + 1];
        System.arraycopy(bounds, 0, this.bounds, 0, bounds.length);
        this.bounds[this.bounds.length - 1] = Histograms.INF_BOUND;

        this.buckets = new AtomicLongArray(this.bounds.length);
    }

    @Override
    public HistogramCollector collect(long value, long count) {
        final int index = Arrays.lowerBound(bounds, value);
        buckets.addAndGet(index, count);
        return this;
    }

    @Override
    public HistogramSnapshot snapshot() {
        return new ExplicitHistogramSnapshot(bounds, Arrays.copyOf(buckets));
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(128);
        sb.append("ExplicitCollector{bounds=[");

        for (int i = 0; i < bounds.length; i++) {
            if (i > 0) {
                sb.append(", ");
            }
            double bound = bounds[i];
            if (bound == Histograms.INF_BOUND) {
                sb.append("inf");
            } else {
                sb.append(bound);
            }
        }

        sb.append("], buckets=");
        Arrays.toString(sb, buckets);

        return sb.append('}')
            .toString();
    }
}
