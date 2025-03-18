package ru.yandex.monlib.metrics.example.push.consumer;

import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;

/**
 * @author Alexey Trushkin
 */
public class DelegateMetricConsumer implements MetricConsumer {
    protected final MetricConsumer target;

    public DelegateMetricConsumer(MetricConsumer target) {
        this.target = target;
    }

    @Override
    public void onStreamBegin(int countHint) {
        target.onStreamBegin(countHint);
    }

    @Override
    public void onStreamEnd() {
        target.onStreamEnd();
    }

    @Override
    public void onCommonTime(long tsMillis) {
        target.onCommonTime(tsMillis);
    }

    @Override
    public void onMetricBegin(MetricType type) {
        target.onMetricBegin(type);
    }

    @Override
    public void onMetricEnd() {
        target.onMetricEnd();
    }

    @Override
    public void onLabelsBegin(int countHint) {
        target.onLabelsBegin(countHint);
    }

    @Override
    public void onLabelsEnd() {
        target.onLabelsEnd();
    }

    @Override
    public void onLabel(Label label) {
        target.onLabel(label);
    }

    @Override
    public void onDouble(long tsMillis, double value) {
        target.onDouble(tsMillis, value);
    }

    @Override
    public void onLong(long tsMillis, long value) {
        target.onLong(tsMillis, value);
    }

    @Override
    public void onHistogram(long tsMillis, HistogramSnapshot snapshot) {
        target.onHistogram(tsMillis, snapshot);
    }
}
