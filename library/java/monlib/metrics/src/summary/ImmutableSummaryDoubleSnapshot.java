package ru.yandex.monlib.metrics.summary;

/**
 * @author Vladimir Gordiychuk
 */
public class ImmutableSummaryDoubleSnapshot implements SummaryDoubleSnapshot {
    public static final SummaryDoubleSnapshot EMPTY =
            new ImmutableSummaryDoubleSnapshot(0, 0, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY);

    private final long count;
    private final double sum;
    private final double min;
    private final double max;
    private final double last;

    public ImmutableSummaryDoubleSnapshot(long count, double sum, double min, double max) {
        this(count, sum, min, max, 0);
    }

    public ImmutableSummaryDoubleSnapshot(long count, double sum, double min, double max, double last) {
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
    public double getSum() {
        return sum;
    }

    @Override
    public double getMin() {
        return min;
    }

    @Override
    public double getMax() {
        return max;
    }

    @Override
    public double getLast() {
        return last;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof SummaryDoubleSnapshot)) {
            return false;
        }
        SummaryDoubleSnapshot that = (SummaryDoubleSnapshot) o;

        if (count != that.getCount()) return false;
        if (Double.compare(that.getSum(), sum) != 0) return false;
        if (Double.compare(that.getMin(), min) != 0) return false;
        if (Double.compare(that.getMax(), max) != 0) return false;
        return Double.compare(that.getLast(), last) == 0;
    }

    @Override
    public int hashCode() {
        int result;
        long temp;
        result = (int) (count ^ (count >>> 32));
        temp = Double.doubleToLongBits(sum);
        result = 31 * result + (int) (temp ^ (temp >>> 32));
        temp = Double.doubleToLongBits(min);
        result = 31 * result + (int) (temp ^ (temp >>> 32));
        temp = Double.doubleToLongBits(max);
        result = 31 * result + (int) (temp ^ (temp >>> 32));
        temp = Double.doubleToLongBits(last);
        result = 31 * result + (int) (temp ^ (temp >>> 32));
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
