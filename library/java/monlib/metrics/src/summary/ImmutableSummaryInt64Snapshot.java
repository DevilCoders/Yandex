package ru.yandex.monlib.metrics.summary;

/**
 * @author Vladimir Gordiychuk
 */
public class ImmutableSummaryInt64Snapshot implements SummaryInt64Snapshot {
    public static final SummaryInt64Snapshot EMPTY =
            new ImmutableSummaryInt64Snapshot(0, 0, Long.MAX_VALUE, Long.MIN_VALUE, 0);

    private final long count;
    private final long sum;
    private final long min;
    private final long max;
    private final long last;

    public ImmutableSummaryInt64Snapshot(long count, long sum, long min, long max) {
        this(count, sum, min, max, 0);
    }

    public ImmutableSummaryInt64Snapshot(long count, long sum, long min, long max, long last) {
        this.count = count;
        this.sum = sum;
        this.min = min;
        this.max = max;
        this.last = last;
    }

    @Override
    public long getCount() {
        return count;
    }

    @Override
    public long getSum() {
        return sum;
    }

    @Override
    public long getMin() {
        return min;
    }

    @Override
    public long getMax() {
        return max;
    }

    @Override
    public long getLast() {
        return last;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof SummaryInt64Snapshot)) {
            return false;
        }

        SummaryInt64Snapshot that = (SummaryInt64Snapshot) o;

        if (count != that.getCount()) return false;
        if (sum != that.getSum()) return false;
        if (min != that.getMin()) return false;
        if (max != that.getMax()) return false;
        return last == that.getLast();
    }

    @Override
    public int hashCode() {
        int result = (int) (count ^ (count >>> 32));
        result = 31 * result + (int) (sum ^ (sum >>> 32));
        result = 31 * result + (int) (min ^ (min >>> 32));
        result = 31 * result + (int) (max ^ (max >>> 32));
        result = 31 * result + (int) (last ^ (last >>> 32));
        return result;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        sb.append("count: ").append(getCount());
        sb.append(", sum: ").append(getSum());
        if (getMin() != Long.MAX_VALUE) {
            sb.append(", min: ").append(getMin());
        }

        if (getMax() != Long.MAX_VALUE) {
            sb.append(", max: ").append(getMax());
        }

        if (getLast() != 0) {
            sb.append(", last: ").append(getLast());
        }
        sb.append('}');
        return sb.toString();
    }
}
