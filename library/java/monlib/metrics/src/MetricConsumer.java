package ru.yandex.monlib.metrics;

import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Sergey Polovko
 */
public interface MetricConsumer {

    void onStreamBegin(int countHint);
    void onStreamEnd();

    void onCommonTime(long tsMillis);

    void onMetricBegin(MetricType type);
    void onMetricEnd();

    void onLabelsBegin(int countHint);
    void onLabelsEnd();
    void onLabel(Label label);

    default void onLabel(String key, String value) {
        onLabel(Labels.allocator.alloc(key, value));
    }

    void onDouble(long tsMillis, double value);
    void onLong(long tsMillis, long value);

    void onHistogram(long tsMillis, HistogramSnapshot snapshot);
}
