package ru.yandex.monlib.metrics.summary;

/**
 * @author Vladimir Gordiychuk
 */
public interface SummaryInt64Snapshot {
    /**
     * Returns the count of values recorded.
     *
     * @return the count of values
     */
    long getCount();

    /**
     * Returns the sum of values recorded, or zero if no values have been
     * recorded.
     *
     * @return the sum of values, or zero if none
     */
    long getSum();

    /**
     * Returns the minimum value recorded, or {@code Long.MAX_VALUE} if no
     * values have been recorded.
     *
     * @return the minimum value, or {@code Long.MAX_VALUE} if none
     */
    long getMin();

    /**
     * Returns the maximum value recorded, or {@code Long.MIN_VALUE} if no
     * values have been recorded
     *
     * @return the maximum value, or {@code Long.MIN_VALUE} if none
     */
    long getMax();

    /**
     * @return latest added value into summary, or zero if none
     */
    long getLast();
}
