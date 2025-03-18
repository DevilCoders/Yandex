package ru.yandex.monlib.metrics;

import ru.yandex.monlib.metrics.MetricConsumerState.State;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;
import ru.yandex.monlib.metrics.series.TimeSeries;


/**
 * @author Sergey Polovko
 */
public class TestMetricConsumer implements MetricConsumer, AutoCloseable {

    private final MetricsData metricsData = new MetricsData();
    private final MetricConsumerState state = new MetricConsumerState();

    // current metric state
    private MetricType metricType = MetricType.UNKNOWN;
    private LabelsBuilder metricLabels = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
    private TimeSeries timeSeries = TimeSeries.empty();


    @Override
    public void onStreamBegin(int countHint) {
        state.swap(State.NOT_STARTED, State.ROOT);
    }

    @Override
    public void onStreamEnd() {
        state.swap(State.ROOT, State.ENDED);
    }

    @Override
    public void onCommonTime(long tsMillis) {
        state.expect(State.ROOT);
        metricsData.setCommonTsMillis(tsMillis);
    }

    @Override
    public void onMetricBegin(MetricType type) {
        state.swap(State.ROOT, State.METRIC);
        this.metricType = type;
    }

    @Override
    public void onMetricEnd() {
        Labels labels = metricLabels.build();
        metricsData.addMetric(labels, metricType, timeSeries);
        clearState();
        state.swap(State.METRIC, State.ROOT);
    }

    @Override
    public void onLabelsBegin(int countHint) {
        switch (state.get()) {
            case ROOT:
                state.set(State.COMMON_LABELS);
                break;
            case METRIC:
                state.set(State.METRIC_LABELS);
                break;
            default:
                state.throwUnexpected();
                break;
        }
    }

    @Override
    public void onLabelsEnd() {
        switch (state.get()) {
            case COMMON_LABELS:
                state.set(State.ROOT);
                break;
            case METRIC_LABELS:
                state.set(State.METRIC);
                break;
            default:
                state.throwUnexpected();
                break;
        }
    }

    @Override
    public void onLabel(Label label) {
        switch (state.get()) {
            case COMMON_LABELS:
                metricsData.addCommonLabel(label);
                break;
            case METRIC_LABELS:
                metricLabels.add(label);
                break;
            default:
                state.throwUnexpected();
                break;
        }
    }

    @Override
    public void onDouble(long tsMillis, double value) {
        state.expect(State.METRIC);
        timeSeries = timeSeries.addDouble(tsMillis, value);
    }

    @Override
    public void onLong(long tsMillis, long value) {
        state.expect(State.METRIC);
        timeSeries = timeSeries.addLong(tsMillis, value);
    }

    @Override
    public void onHistogram(long tsMillis, HistogramSnapshot value) {
        state.expect(State.METRIC);
        timeSeries = timeSeries.addHistogram(tsMillis, value);
    }

    public MetricsData getMetricsData() {
        return metricsData;
    }

    private void clearState() {
        metricType = MetricType.UNKNOWN;
        metricLabels.clear();
        timeSeries = TimeSeries.empty();
    }

    @Override
    public void close() {
        state.expect(State.ENDED);
    }
}
