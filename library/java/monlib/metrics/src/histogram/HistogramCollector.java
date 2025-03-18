package ru.yandex.monlib.metrics.histogram;

/**
 * @author Sergey Polovko
 */
public interface HistogramCollector {

    /**
     * Store given {@code value} in this collector.
     */
    default HistogramCollector collect(long value) {
        return collect(value, 1);
    }

    /**
     * Store {@code count} times given {@code value} in this collector.
     * @return this collector (for a fluent interface)
     */
    HistogramCollector collect(long value, long count);

    /**
     * Add counts from snapshot into this collector
     */
    default HistogramCollector collect(HistogramSnapshot snapshot) {
        for (int index = 0; index < snapshot.count(); index++) {
            collect(Math.round(snapshot.upperBound(index)), snapshot.value(index));
        }

        return this;
    }

    default HistogramCollector combine(HistogramCollector collector) {
        return collect(collector.snapshot());
    }

    /**
     * @return snapshot of the state of this collector.
     */
    HistogramSnapshot snapshot();
}
