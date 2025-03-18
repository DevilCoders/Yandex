package ru.yandex.monlib.metrics.histogram;

import java.util.concurrent.atomic.AtomicLongArray;


/**
 * @author Sergey Polovko
 */
final class ExponentialCollector implements HistogramCollector {

    private final AtomicLongArray buckets;
    private final double base;
    private final double scale;
    private final long minValue;
    private final long maxValue;
    private final double logOfBase;

    ExponentialCollector(int bucketsCount, double base, double scale) {
        this.buckets = new AtomicLongArray(bucketsCount);
        this.base = base;
        this.scale = scale;
        this.minValue = Math.round(scale);
        this.maxValue = Math.round(scale * Math.pow(base, bucketsCount - 2));
        this.logOfBase = Math.log(base);
    }

    @Override
    public HistogramCollector collect(long value, long count) {
        final int index;
        if (value <= minValue) {
            index = 0;
        } else if (value > maxValue) {
            index = buckets.length() - 1;
        } else {
            final double logBase = Math.log(value / scale) / logOfBase;
            index = Math.toIntExact((long) Math.ceil(logBase));
        }
        buckets.addAndGet(index, count);
        return this;
    }

    @Override
    public HistogramSnapshot snapshot() {
        return new Snapshot(Arrays.copyOf(buckets), base, scale);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(128);
        sb.append("ExponentialCollector{")
            .append("base=").append(base)
            .append(", scale=").append(scale)
            .append(", buckets=");

        Arrays.toString(sb, buckets);

        return sb.append('}')
            .toString();
    }

    private static final class Snapshot extends AbstractHistogramSnapshot {
        private final double base;
        private final double scale;

        Snapshot(long[] buckets, double base, double scale) {
            super(buckets);
            this.base = base;
            this.scale = scale;
        }

        @Override
        public double upperBound(int index) {
            if (index == count() - 1) {
                return Histograms.INF_BOUND;
            }
            return Math.round(scale * Math.pow(base, index));
        }

        @Override
        public boolean boundsEquals(HistogramSnapshot rhs) {
            if (rhs instanceof Snapshot) {
                Snapshot snapshot = (Snapshot) rhs;
                return Double.compare(snapshot.base, base) == 0 &&
                    Double.compare(snapshot.scale, scale) == 0;
            }
            return super.boundsEquals(rhs);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            Snapshot snapshot = (Snapshot) o;
            return
                Double.compare(snapshot.base, base) == 0 &&
                Double.compare(snapshot.scale, scale) == 0 &&
                fastBucketsEquals(snapshot);
        }

        @Override
        public int hashCode() {
            int result = Double.hashCode(base);
            result = 31 * result + Double.hashCode(scale);
            return 31 * result + bucketsHashCode();
        }
    }
}
