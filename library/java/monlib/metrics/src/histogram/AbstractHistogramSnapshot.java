package ru.yandex.monlib.metrics.histogram;

import java.util.Arrays;


/**
 * @author Sergey Polovko
 */
abstract class AbstractHistogramSnapshot implements HistogramSnapshot {

    private final long[] buckets;

    public AbstractHistogramSnapshot(long[] buckets) {
        this.buckets = buckets;
        if (buckets.length > Histograms.MAX_BUCKETS_COUNT) {
            throw new IllegalArgumentException("bucketsCount must <= " + Histograms.MAX_BUCKETS_COUNT + ", got: " + buckets.length);
        }
    }

    @Override
    public int count() {
        return buckets.length;
    }

    @Override
    public long value(int index) {
        return buckets[index];
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        int i = 0;
        for (; i < count() - 1; i++) {
            sb.append(Doubles.toString(upperBound(i))).append(": ").append(value(i));
            sb.append(", ");
        }

        if (count() != i) {
            if (Double.compare(upperBound(i), Histograms.INF_BOUND) == 0) {
                sb.append("inf: ").append(value(i));
            } else {
                sb.append(Doubles.toString(upperBound(i))).append(": ").append(value(i));
            }
        }
        sb.append('}');
        return sb.toString();
    }

    @Override
    public boolean boundsEquals(HistogramSnapshot rhs) {
        // slow pass
        if (count() != rhs.count()) {
            return false;
        }
        for (int i = 0; i < count(); i++) {
            if (upperBound(i) != rhs.upperBound(i)) {
                return false;
            }
        }
        return true;
    }

    boolean fastBucketsEquals(AbstractHistogramSnapshot rhs) {
        return Arrays.equals(buckets, rhs.buckets);
    }

    @Override
    public boolean bucketsEquals(HistogramSnapshot rhs) {
        // (1) fast pass
        if (rhs instanceof AbstractHistogramSnapshot) {
            return fastBucketsEquals((AbstractHistogramSnapshot) rhs);
        }
        // (2) slow pass
        if (count() != rhs.count()) {
            return false;
        }
        for (int i = 0; i < count(); i++) {
            if (value(i) != rhs.value(i)) {
                return false;
            }
        }
        return true;
    }

    int bucketsHashCode() {
        return Arrays.hashCode(buckets);
    }
}
