package ru.yandex.monlib.metrics.histogram;

import java.util.Arrays;

/**
 * @author Vladimir Gordiychuk
 */
public class ExplicitHistogramSnapshot extends AbstractHistogramSnapshot {
    public static HistogramSnapshot EMPTY = new ExplicitHistogramSnapshot(new double[0], new long[0]);
    private final double[] bounds;

    public ExplicitHistogramSnapshot(double[] bounds, long[] buckets) {
        super(buckets);
        if (bounds.length != buckets.length) {
            throw new IllegalArgumentException("boundsSize(" + bounds.length + ") != bucketsSize(" + buckets.length + ") ");
        }
        this.bounds = bounds;
    }

    @Override
    public double upperBound(int index) {
        return bounds[index];
    }

    @Override
    public boolean boundsEquals(HistogramSnapshot rhs) {
        if (rhs instanceof ExplicitHistogramSnapshot) {
            return java.util.Arrays.equals(bounds, ((ExplicitHistogramSnapshot) rhs).bounds);
        }
        return super.boundsEquals(rhs);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof HistogramSnapshot)) {
            return false;
        }

        HistogramSnapshot that = (HistogramSnapshot) o;
        return boundsEquals(that) && bucketsEquals(that);
    }

    @Override
    public int hashCode() {
        int result = bucketsHashCode();
        result = 31 * result + Arrays.hashCode(bounds);
        return result;
    }
}
