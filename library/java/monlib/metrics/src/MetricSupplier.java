package ru.yandex.monlib.metrics;

import ru.yandex.monlib.metrics.labels.Labels;

/**
 * @author Vladimir Gordiychuk
 */
public interface MetricSupplier {

    /**
     * Estimates metric count that this supplier can provide.
     *
     * @return estimated metric count or zero if their count is unknown
     */
    int estimateCount();

    /**
     * Inits metric stream, supplies all known metrics for the given consumer and closes stream.
     *
     * @param tsMillis common time that will be used for all supplied metric values
     * @param consumer target metric consumer
     */
    default void supply(long tsMillis, MetricConsumer consumer) {
        consumer.onStreamBegin(estimateCount());
        append(tsMillis, Labels.empty(), consumer);
        consumer.onStreamEnd();
    }

    /**
     * Append metrics from current supplier to already active metric consumer. It's able to append
     * metrics from different {@link MetricSupplier}s into single consumer.
     *
     * @param tsMillis     common time, or zero if already was provided for consumer, supplier can
     *                     ignore it and put into consumer own time.
     * @param commonLabels common labels that should be append to each metric from this supplier
     * @param consumer     target metric consumer
     */
    void append(long tsMillis, Labels commonLabels, MetricConsumer consumer);
}
