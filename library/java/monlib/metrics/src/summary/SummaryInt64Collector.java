package ru.yandex.monlib.metrics.summary;

/**
 * @author Vladimir Gordiychuk
 */
public interface SummaryInt64Collector {
    /**
     * Store given {@code value} in this collector.
     */
    SummaryInt64Collector collect(long value);

    /**
     * @return snapshot of the state of this collector.
     */
    SummaryInt64Snapshot snapshot();
}
