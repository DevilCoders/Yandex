package ru.yandex.monlib.metrics.encode.text;

import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.Writer;
import java.time.Instant;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.MetricConsumerState;
import ru.yandex.monlib.metrics.MetricConsumerState.State;
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
public class MetricTextEncoder implements MetricEncoder {

    private static final String LINE_PATTERN = "%" + computeMaxTypeNameLength() + "s %s%s";

    private final PrintWriter writer;
    private final boolean humanReadableTime;
    private final MetricConsumerState state = new MetricConsumerState();

    private long commonTsMillis;

    // last metric state
    private LabelsBuilder lastLabels = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
    private MetricType lastType = MetricType.UNKNOWN;
    private TimeSeries timeSeries = TimeSeries.empty();

    public MetricTextEncoder(OutputStream out, boolean humanReadableTime) {
        this.writer = new PrintWriter(out);
        this.humanReadableTime = humanReadableTime;
    }

    public MetricTextEncoder(Writer out, boolean humanReadableTime) {
        this.writer = new PrintWriter(out);
        this.humanReadableTime = humanReadableTime;
    }

    private void clearLastState() {
        lastType = MetricType.UNKNOWN;
        lastLabels = new LabelsBuilder(Labels.MAX_LABELS_COUNT);
        timeSeries = TimeSeries.empty();
    }

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
        commonTsMillis = tsMillis;
        if (tsMillis != 0) {
            writer.print("common time: ");
            printTime(commonTsMillis);
            writer.println();
        }
    }

    @Override
    public void onMetricBegin(MetricType type) {
        state.swap(State.ROOT, State.METRIC);
        clearLastState();
        lastType = type;
    }

    @Override
    public void onMetricEnd() {
        state.swap(State.METRIC, State.ROOT);
        final String name;
        final int nameIdx = lastLabels.indexOf("sensor");
        if (nameIdx >= 0) {
            String metricName = lastLabels.at(nameIdx).getValue();
            lastLabels.remove(nameIdx);
            name = needQuotes(metricName) ? '"' + metricName + '"' : metricName;
        } else {
            name = "";
        }

        final Labels labels = lastLabels.build();
        writer.printf(LINE_PATTERN, lastType.name(), name, labels);
        if (!timeSeries.isEmpty()) {
            writer.print(" [");
            for (int i = 0; i < timeSeries.size(); i++) {
                if (i > 0) {
                    writer.print(", ");
                }

                final long tsMillis = timeSeries.tsMillisAt(i);
                if (tsMillis != 0 && tsMillis != commonTsMillis) {
                    writer.print('(');
                    printTime(tsMillis);
                    writer.print(", ");
                    printValue(i);
                    writer.print(')');
                } else {
                    printValue(i);
                }
            }
            writer.print(']');
        }
        writer.println();
    }

    private void printTime(long tsMillis) {
        if (humanReadableTime) {
            writer.print(Instant.ofEpochMilli(tsMillis));
        } else {
            writer.print(TimeUnit.MILLISECONDS.toSeconds(tsMillis));
        }
    }

    private void printValue(int i) {
        if (timeSeries.isDouble()) {
            writer.print(timeSeries.doubleAt(i));
        } else if (timeSeries.isLong()) {
            writer.print(timeSeries.longAt(i));
        } else if (timeSeries.isHistogram()) {
            writer.print(timeSeries.histogramAt(i));
        }
    }

    private static boolean needQuotes(String name) {
        return name.indexOf(' ') >= 0;
    }

    @Override
    public void onLabelsBegin(int countHint) {
        final State s = state.get();
        if (s == State.ROOT) {
            state.set(State.COMMON_LABELS);
        } else if (s == State.METRIC) {
            state.set(State.METRIC_LABELS);
        }
    }

    @Override
    public void onLabelsEnd() {
        final State s = this.state.get();
        if (s == State.COMMON_LABELS) {
            state.set(State.ROOT);
            writer.println("common labels: " + lastLabels.build());
        } else if (s == State.METRIC_LABELS) {
            state.set(State.METRIC);
        }
    }

    @Override
    public void onLabel(Label label) {
        final State s = state.get();
        if (s != State.COMMON_LABELS && s != State.METRIC_LABELS) {
            state.throwUnexpected();
        }
        lastLabels.add(label);
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
        writer.close();
    }

    private static int computeMaxTypeNameLength() {
        return Arrays.stream(MetricType.values())
            .filter(k -> k != MetricType.LOG_HISTOGRAM)
            .mapToInt(type -> type.name().length())
            .max()
            .orElse(0);
    }
}
