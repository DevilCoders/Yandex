package ru.yandex.monlib.metrics.encode.json;

import java.io.OutputStream;
import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.MetricConsumerState;
import ru.yandex.monlib.metrics.MetricConsumerState.State;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.series.TimeSeries;


/**
 * @author Sergey Polovko
 */
public class MetricJsonEncoder implements MetricEncoder {

    private final JsonWriter w;
    private final MetricConsumerState state = new MetricConsumerState();

    // last metric state
    private MetricType lastType = MetricType.UNKNOWN;
    private TimeSeries timeSeries = TimeSeries.empty();

    public MetricJsonEncoder(OutputStream out) {
        this.w = new JsonWriter(out, 8 << 10); // 8 KiB
    }

    private void clearLastState() {
        lastType = MetricType.UNKNOWN;
        timeSeries = TimeSeries.empty();
    }

    @Override
    public void onStreamBegin(int countHint) {
        state.swap(State.NOT_STARTED, State.ROOT);
        w.objectBegin();
    }

    @Override
    public void onStreamEnd() {
        if (state.get() == State.METRICS_ARRAY) {
            w.arrayEnd();
            state.set(State.ENDED);
        } else {
            state.swap(State.ROOT, State.ENDED);
        }
        w.objectEnd();
    }

    @Override
    public void onCommonTime(long tsMillis) {
        state.expect(State.ROOT);
        if (tsMillis != 0) {
            w.key(JsonConstant.TS.getBytes());
            w.numberValue(TimeUnit.MILLISECONDS.toSeconds(tsMillis));
        }
    }

    @Override
    public void onMetricBegin(MetricType type) {
        if (state.get() == State.ROOT) {
            state.swap(State.ROOT, State.METRICS_ARRAY);
            w.key(JsonConstant.SENSORS.getBytes());
            w.arrayBegin();
        }

        state.swap(State.METRICS_ARRAY, State.METRIC);
        clearLastState();
        lastType = type;
        w.objectBegin();
        writeType();
    }

    @Override
    public void onMetricEnd() {
        state.swap(State.METRIC, State.METRICS_ARRAY);
        writeTimeSeries();
        w.objectEnd();
    }

    private void writeType() {
        if (lastType == MetricType.UNKNOWN) {
            return;
        }

        // TODO: replace with type
        w.key(JsonConstant.KIND.getBytes());
        switch (lastType) {
            case DGAUGE:
                w.stringValue(JsonConstant.DGAUGE.getBytes());
                break;
            case IGAUGE:
                w.stringValue(JsonConstant.IGAUGE.getBytes());
                break;
            case COUNTER:
                w.stringValue(JsonConstant.COUNTER.getBytes());
                break;
            case RATE:
                w.stringValue(JsonConstant.RATE.getBytes());
                break;
            case HIST:
                w.stringValue(JsonConstant.HIST.getBytes());
                break;
            case HIST_RATE:
                w.stringValue(JsonConstant.HIST_RATE.getBytes());
                break;
            default:
                throw new IllegalArgumentException("unsupported metric type: " + lastType);
        }
    }

    private void writeTimeSeries() {
        if (timeSeries.isEmpty()) {
            return;
        }

        if (timeSeries.size() == 1) {
            writePoint(0);
        } else {
            w.key(JsonConstant.TIMESERIES.getBytes());
            w.arrayBegin();
            for (int index = 0; index < timeSeries.size(); index++) {
                w.objectBegin();
                writePoint(index);
                w.objectEnd();
            }
            w.arrayEnd();
        }
    }

    private void writePoint(int i) {
        writeTs(i);
        writeValue(i);
    }

    private void writeTs(int i) {
        long tsMillis = timeSeries.tsMillisAt(i);
        if (tsMillis != 0) {
            w.key(JsonConstant.TS.getBytes());
            w.numberValue(TimeUnit.MILLISECONDS.toSeconds(tsMillis));
        }
    }

    private void writeValue(int i) {
        if (timeSeries.isDouble()) {
            w.key(JsonConstant.VALUE.getBytes());
            w.numberValue(timeSeries.doubleAt(i));
        } else if (timeSeries.isLong()) {
            w.key(JsonConstant.VALUE.getBytes());
            w.numberValue(timeSeries.longAt(i));
        } else if (timeSeries.isHistogram()) {
            w.key(JsonConstant.HIST_FIELD.getBytes());
            writeHistogram(i);
        }
    }

    private void writeHistogram(int i) {
        HistogramSnapshot snapshot = timeSeries.histogramAt(i);
        w.objectBegin();
        if (snapshot.count() != 0) {
            boolean inf = Double.compare(snapshot.upperBound(snapshot.count() - 1), Histograms.INF_BOUND) == 0;
            int count = inf ? snapshot.count() - 1 : snapshot.count();

            w.key(JsonConstant.BOUNDS.getBytes());
            w.arrayBegin();
            for (int index = 0; index < count; index++) {
                w.numberValue(snapshot.upperBound(index));
            }
            w.arrayEnd();

            w.key(JsonConstant.BUCKETS.getBytes());
            w.arrayBegin();
            for (int index = 0; index < count; index++) {
                w.numberValue(snapshot.value(index));
            }
            w.arrayEnd();

            if (inf) {
                w.key(JsonConstant.INF.getBytes());
                w.numberValue(snapshot.value(snapshot.count() - 1));
            }
        }
        w.objectEnd();
    }

    @Override
    public void onLabelsBegin(int countHint) {
        final State s = state.get();
        if (s == State.ROOT) {
            state.set(State.COMMON_LABELS);
        } else if (s == State.METRIC) {
            state.set(State.METRIC_LABELS);
        } else {
            state.throwUnexpected();
        }
        w.key(JsonConstant.LABELS.getBytes());
        w.objectBegin();
    }

    @Override
    public void onLabelsEnd() {
        final State s = this.state.get();
        if (s == State.COMMON_LABELS) {
            state.set(State.ROOT);
        } else if (s == State.METRIC_LABELS) {
            state.set(State.METRIC);
        } else {
            state.throwUnexpected();
        }
        w.objectEnd();
    }

    @Override
    public void onLabel(Label label) {
        final State s = state.get();
        if (s != State.COMMON_LABELS && s != State.METRIC_LABELS) {
            state.throwUnexpected();
        }
        w.keyValue(label);
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

    public void onHistogram(long tsMillis, HistogramSnapshot snapshot) {
        state.expect(State.METRIC);
        timeSeries = timeSeries.addHistogram(tsMillis, snapshot);
    }

    @Override
    public void close() {
        state.expect(State.ENDED);
        w.close();
    }
}
