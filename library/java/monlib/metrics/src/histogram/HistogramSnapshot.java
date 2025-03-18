package ru.yandex.monlib.metrics.histogram;

/**
 * @author Sergey Polovko
 */
public interface HistogramSnapshot {

    /**
     * @return buckets count.
     */
    int count();

    /**
     * @return upper bound for bucket with particular index.
     */
    double upperBound(int index);

    /**
     * @return value stored in bucket with particular index.
     */
    long value(int index);

    /**
     * @return true iff snapshots have equal buckets' upper bounds.
     */
    boolean boundsEquals(HistogramSnapshot rhs);

    /**
     * @return true iff snapshots have equal values in buckets.
     */
    boolean bucketsEquals(HistogramSnapshot rhs);
}
