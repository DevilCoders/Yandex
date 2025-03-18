package ru.yandex.monlib.metrics.summary;

/**
 * @author Vladimir Gordiychuk
 */
public interface SummaryDoubleCollector {
    /**
     * Store given {@code value} in this collector.
     */
    SummaryDoubleCollector collect(double value);

    /**
     * @return snapshot of the state of this collector.
     */
    SummaryDoubleSnapshot snapshot();
}
