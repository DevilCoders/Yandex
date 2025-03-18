package ru.yandex.monlib.metrics.encode.prometheus;

import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;

import ru.yandex.monlib.metrics.MetricConsumerState;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;
import ru.yandex.monlib.metrics.series.TimeSeries;


/**
 * @author Sergey Polovko
 */
public class MetricPrometheusEncoder implements MetricEncoder {

    private final PrometheusWriterImpl writer;
    private final MetricConsumerState state = new MetricConsumerState();

    private long commonTsMillis;
    private Labels commonLabels = Labels.empty();

    // last metric state
    private LabelsBuilder lastLabels = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
    private MetricType lastType = MetricType.UNKNOWN;
    private TimeSeries timeSeries = TimeSeries.empty();

    public MetricPrometheusEncoder(OutputStream out) {
        this.writer = new PrometheusWriterImpl(new OutputStreamWriter(out));
    }

    public MetricPrometheusEncoder(Writer writer) {
        this.writer = new PrometheusWriterImpl(writer);
    }

    private void clearLastState() {
        lastType = MetricType.UNKNOWN;
        lastLabels = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
        timeSeries = TimeSeries.empty();
    }

    @Override
    public void onStreamBegin(int countHint) {
        state.swap(MetricConsumerState.State.NOT_STARTED, MetricConsumerState.State.ROOT);
    }

    @Override
    public void onStreamEnd() {
        state.swap(MetricConsumerState.State.ROOT, MetricConsumerState.State.ENDED);
    }

    @Override
    public void onCommonTime(long tsMillis) {
        state.expect(MetricConsumerState.State.ROOT);
        commonTsMillis = tsMillis;
    }

    @Override
    public void onMetricBegin(MetricType type) {
        state.swap(MetricConsumerState.State.ROOT, MetricConsumerState.State.METRIC);
        clearLastState();
        lastType = type;
    }

    @Override
    public void onMetricEnd() {
        state.swap(MetricConsumerState.State.METRIC, MetricConsumerState.State.ROOT);

        if (timeSeries.isEmpty()) {
            return;
        }

        final String name;
        final int nameIdx = lastLabels.indexOf("sensor");
        if (nameIdx >= 0) {
            String metricName = lastLabels.at(nameIdx).getValue();
            lastLabels.remove(nameIdx);
            name = metricName;
        } else {
            throw new IllegalStateException("labels " + lastLabels + " does not contain label 'sensor'");
        }

        writer.writeType(lastType, name);

        final Labels labels = commonLabels.addAll(lastLabels.build());

        final int lastIdx = timeSeries.size() - 1;
        long tsMillis = timeSeries.tsMillisAt(lastIdx);
        if (tsMillis == 0) {
            tsMillis = commonTsMillis;
        }

        // because Prometheus does not support multiple points per one metric
        // here we write only the last point from timeseries

        if (timeSeries.isDouble()) {
            double value = timeSeries.doubleAt(lastIdx);
            writer.writeValue(name, labels, tsMillis, value);
        } else if (timeSeries.isLong()) {
            long value = timeSeries.longAt(lastIdx);
            writer.writeValue(name, labels, tsMillis, (double) value);
        } else if (timeSeries.isHistogram()) {
            if (labels.hasKey(PrometheusModel.BUCKET_LABEL)) {
                String msg = String.format(
                    "labels %s of metric %s have reserved label '%s'",
                    labels, name, PrometheusModel.BUCKET_LABEL);
                throw new IllegalStateException(msg);
            }
            HistogramSnapshot value = timeSeries.histogramAt(lastIdx);
            writer.writeHistogram(name, labels, tsMillis, value);
        } else {
            throw new IllegalStateException("unsupported timeseries payload, name: " + name);
        }
    }

    @Override
    public void onLabelsBegin(int countHint) {
        final MetricConsumerState.State s = state.get();
        if (s == MetricConsumerState.State.ROOT) {
            state.set(MetricConsumerState.State.COMMON_LABELS);
        } else if (s == MetricConsumerState.State.METRIC) {
            state.set(MetricConsumerState.State.METRIC_LABELS);
        }
    }

    @Override
    public void onLabelsEnd() {
        final MetricConsumerState.State s = this.state.get();
        if (s == MetricConsumerState.State.COMMON_LABELS) {
            state.set(MetricConsumerState.State.ROOT);
            commonLabels = lastLabels.build();
        } else if (s == MetricConsumerState.State.METRIC_LABELS) {
            state.set(MetricConsumerState.State.METRIC);
        }
    }

    @Override
    public void onLabel(Label label) {
        final MetricConsumerState.State s = state.get();
        if (s != MetricConsumerState.State.COMMON_LABELS && s != MetricConsumerState.State.METRIC_LABELS) {
            state.throwUnexpected();
        }
        lastLabels.add(label);
    }

    @Override
    public void onDouble(long tsMillis, double value) {
        state.expect(MetricConsumerState.State.METRIC);
        timeSeries = timeSeries.addDouble(tsMillis, value);
    }

    @Override
    public void onLong(long tsMillis, long value) {
        state.expect(MetricConsumerState.State.METRIC);
        timeSeries = timeSeries.addLong(tsMillis, value);
    }

    public void onHistogram(long tsMillis, HistogramSnapshot snapshot) {
        state.expect(MetricConsumerState.State.METRIC);
        timeSeries = timeSeries.addHistogram(tsMillis, snapshot);
    }

    @Override
    public void close() {
        state.expect(MetricConsumerState.State.ENDED);
        writer.close();
    }
}
