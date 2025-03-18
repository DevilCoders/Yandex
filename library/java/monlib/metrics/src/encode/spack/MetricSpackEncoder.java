package ru.yandex.monlib.metrics.encode.spack;

import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.MetricConsumerState;
import ru.yandex.monlib.metrics.MetricConsumerState.State;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.encode.MetricEncoder;
import ru.yandex.monlib.metrics.encode.spack.StringPoolBuilder.PooledString;
import ru.yandex.monlib.metrics.encode.spack.compression.EncodeStream;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.MetricTypes;
import ru.yandex.monlib.metrics.encode.spack.format.MetricValuesType;
import ru.yandex.monlib.metrics.encode.spack.format.SpackHeader;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;
import ru.yandex.monlib.metrics.series.TimeSeries;


/**
 * @author Sergey Polovko
 */
public class MetricSpackEncoder implements MetricEncoder {
    private static final PooledString[] EMPTY = {};

    private final TimePrecision timePrecision;
    private final CompressionAlg compressionAlg;
    private final EncodeStream out;

    // common data
    private long commonTsMillis;
    private PooledString[] commonLabels = EMPTY;

    // string pools
    private final StringPoolBuilder keysPool = new StringPoolBuilder();
    private final StringPoolBuilder valuesPool = new StringPoolBuilder();

    // metrics data
    private List<MetricData> metrics;
    private MetricData lastMetric;

    // stats
    private int metricCount = 0;
    private int pointsCount = 0;

    // helpers
    private final MetricConsumerState state = new MetricConsumerState();
    private final LabelsBuilder labelsBuilder = new LabelsBuilder(Labels.MAX_LABELS_COUNT);

    public MetricSpackEncoder(
        TimePrecision timePrecision,
        CompressionAlg compressionAlg,
        OutputStream out)
    {
        this.timePrecision = timePrecision;
        this.compressionAlg = compressionAlg;
        this.out = EncodeStream.create(compressionAlg, out);
    }

    @Override
    public void onStreamBegin(int countHint) {
        state.swap(State.NOT_STARTED, State.ROOT);
        this.metrics = new ArrayList<>(countHint > 0 ? countHint : 32);
    }

    @Override
    public void onStreamEnd() {
        state.swap(State.ROOT, State.ENDED);
    }

    @Override
    public void onCommonTime(long tsMillis) {
        state.expect(State.ROOT);
        commonTsMillis = tsMillis;
    }

    @Override
    public void onMetricBegin(MetricType type) {
        state.swap(State.ROOT, State.METRIC);
        labelsBuilder.clear();
        lastMetric = new MetricData(type);
    }

    @Override
    public void onMetricEnd() {
        state.swap(State.METRIC, State.ROOT);
        if (labelsBuilder.isEmpty()) {
            throw new IllegalStateException("metric without labels");
        }
        lastMetric.labels = toPooledStrings(labelsBuilder);
        metrics.add(lastMetric);
        metricCount++;
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
        }
    }

    @Override
    public void onLabelsEnd() {
        switch (state.get()) {
            case COMMON_LABELS:
                state.set(State.ROOT);
                commonLabels = toPooledStrings(labelsBuilder);
                break;
            case METRIC_LABELS:
                state.set(State.METRIC);
                break;
        }
    }

    @Override
    public void onLabel(Label label) {
        labelsBuilder.add(label);
    }

    @Override
    public void onDouble(long tsMillis, double value) {
        lastMetric.timeSeries = lastMetric.timeSeries.addDouble(tsMillis, value);
        pointsCount++;
    }

    @Override
    public void onLong(long tsMillis, long value) {
        lastMetric.timeSeries = lastMetric.timeSeries.addLong(tsMillis, value);
        pointsCount++;
    }

    @Override
    public void onHistogram(long tsMillis, HistogramSnapshot value) {
        lastMetric.timeSeries = lastMetric.timeSeries.addHistogram(tsMillis, value);
        pointsCount++;
    }

    @Override
    public void close() {
        state.expect(State.ENDED);

        keysPool.sortByFrequencies();
        valuesPool.sortByFrequencies();

        // (1) header
        SpackHeader header = new SpackHeader(
            timePrecision,
            compressionAlg,
            keysPool.getBytesSize() + keysPool.size(),
            valuesPool.getBytesSize() + valuesPool.size(),
            metricCount,
            pointsCount);
        out.writeHeader(header);

        // (2) string pools
        keysPool.forEachString(out::writeString);
        valuesPool.forEachString(out::writeString);

        // (3) common time
        writeTime(commonTsMillis);

        // (4) common labels' indexes
        writeLabels(commonLabels);

        // (5) metrics
        for (MetricData metric : metrics) {
            // (5.1) types byte
            out.writeByte(packTypes(metric));

            // (5.2) flags byte
            // TODO: implement
            out.writeByte((byte) 0x00);

            // (5.3) labels
            writeLabels(metric.labels);

            // (5.4) points
            writeTimeSeries(metric.timeSeries);
        }

        out.close();
    }

    private void writeTimeSeries(TimeSeries timeSeries) {
        switch (timeSeries.size()) {
            case 0:
                break;

            case 1: {
                final long tsMillis = timeSeries.tsMillisAt(0);
                if (tsMillis != 0) {
                    writeTime(tsMillis);
                }
                if (timeSeries.isDouble()) {
                    out.writeDouble(timeSeries.doubleAt(0));
                } else if (timeSeries.isLong()) {
                    out.writeLongLe(timeSeries.longAt(0));
                } else if (timeSeries.isHistogram()) {
                    writeHistogram(timeSeries.histogramAt(0));
                }
                break;
            }

            default: {
                out.writeVarint32(timeSeries.size());
                if (timeSeries.isDouble()) {
                    for (int i = 0; i < timeSeries.size(); i++) {
                        writeTime(timeSeries.tsMillisAt(i));
                        out.writeDouble(timeSeries.doubleAt(i));
                    }
                } else if (timeSeries.isLong()) {
                    for (int i = 0; i < timeSeries.size(); i++) {
                        writeTime(timeSeries.tsMillisAt(i));
                        out.writeLongLe(timeSeries.longAt(i));
                    }
                } else if (timeSeries.isHistogram()) {
                    for (int i = 0; i < timeSeries.size(); i++) {
                        writeTime(timeSeries.tsMillisAt(i));
                        writeHistogram(timeSeries.histogramAt(i));
                    }
                }
                break;
            }
        }
    }

    private void writeHistogram(HistogramSnapshot snapshot) {
        out.writeVarint32(snapshot.count());
        for (int index = 0; index < snapshot.count(); index++) {
            out.writeDouble(snapshot.upperBound(index));
        }
        for (int index = 0; index < snapshot.count(); index++) {
            out.writeLongLe(snapshot.value(index));
        }
    }

    private void writeLabels(PooledString[] labels) {
        out.writeVarint32(labels.length >>> 1);
        for (PooledString label : labels) {
            out.writeVarint32(label.index);
        }
    }

    private void writeTime(long tsMillis) {
        if (timePrecision == TimePrecision.SECONDS) {
            out.writeIntLe((int) TimeUnit.MILLISECONDS.toSeconds(tsMillis));
        } else {
            out.writeLongLe(tsMillis);
        }
    }

    private byte packTypes(MetricData metric) {
        final MetricValuesType valuesType;
        switch (metric.timeSeries.size()) {
            case 0:
                valuesType = MetricValuesType.NONE;
                break;
            case 1:
                valuesType = (metric.timeSeries.tsMillisAt(0) == 0)
                    ? MetricValuesType.ONE_WITHOUT_TS
                    : MetricValuesType.ONE_WITH_TS;
                break;
            default:
                valuesType = MetricValuesType.MANY_WITH_TS;
                break;
        }
        return MetricTypes.pack(metric.type, valuesType);
    }

    private PooledString[] toPooledStrings(LabelsBuilder labels) {
        final PooledString[] pooledStrings = new PooledString[labels.size() << 1];
        for (int i = 0; i < labels.size(); i++) {
            final Label label = labels.at(i);
            pooledStrings[i << 1] = keysPool.putIfAbsent(label.getKey());
            pooledStrings[(i << 1) + 1] = valuesPool.putIfAbsent(label.getValue());
        }
        return pooledStrings;
    }

    private static final class MetricData {
        private final MetricType type;
        private TimeSeries timeSeries = TimeSeries.empty();
        private PooledString[] labels = EMPTY;

        public MetricData(MetricType type) {
            this.type = type;
        }
    }
}
