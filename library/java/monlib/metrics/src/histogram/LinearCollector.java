package ru.yandex.monlib.metrics.histogram;

import java.util.concurrent.atomic.AtomicLongArray;


/**
 * @author Sergey Polovko
 */
final class LinearCollector implements HistogramCollector {

    private final long startValue;
    private final long bucketWidth;
    private final AtomicLongArray buckets;
    private final long maxValue;

    LinearCollector(int bucketsCount, long startValue, long bucketWidth) {
        this.startValue = startValue;
        this.bucketWidth = bucketWidth;
        this.buckets = new AtomicLongArray(bucketsCount);
        this.maxValue = startValue + bucketWidth * (bucketsCount - 2);
    }

    @Override
    public HistogramCollector collect(long value, long count) {
        final int index;
        if (value <= startValue) {
            index = 0;
        } else if (value > maxValue) {
            index = buckets.length() - 1;
        } else {
            index = Math.toIntExact((long) Math.ceil((double) (value - startValue) / bucketWidth));
        }
        buckets.addAndGet(index, count);
        return this;
    }

    @Override
    public HistogramSnapshot snapshot() {
        return new Snapshot(startValue, bucketWidth, Arrays.copyOf(buckets));
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(128);
        sb.append("LinearCollector{")
            .append("startValue=").append(startValue)
            .append(", bucketWidth=").append(bucketWidth)
            .append(", buckets=");

        Arrays.toString(sb, buckets);

        return sb.append('}')
            .toString();
    }

    private static final class Snapshot extends AbstractHistogramSnapshot {
        private final long startValue;
        private final long bucketWidth;

        Snapshot(long startValue, long bucketWidth, long[] buckets) {
            super(buckets);
            this.startValue = startValue;
            this.bucketWidth = bucketWidth;
        }

        @Override
        public double upperBound(int index) {
            if (index == count() - 1) {
                return Histograms.INF_BOUND;
            }
            return startValue + bucketWidth * index;
        }

        @Override
        public boolean boundsEquals(HistogramSnapshot rhs) {
            if (rhs instanceof Snapshot) {
                Snapshot snapshot = (Snapshot) rhs;
                return snapshot.startValue == startValue &&
                    snapshot.bucketWidth == bucketWidth;
            }
            return super.boundsEquals(rhs);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            Snapshot snapshot = (Snapshot) o;
            return
                snapshot.startValue == startValue &&
                snapshot.bucketWidth == bucketWidth &&
                fastBucketsEquals(snapshot);
        }

        @Override
        public int hashCode() {
            int result = Long.hashCode(startValue);
            result = 31 * result + Long.hashCode(bucketWidth);
            return 31 * result + bucketsHashCode();
        }
    }
}
