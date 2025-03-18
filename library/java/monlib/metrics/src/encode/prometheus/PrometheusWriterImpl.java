package ru.yandex.monlib.metrics.encode.prometheus;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.io.Writer;
import java.util.HashMap;
import java.util.Map;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * See <a href="https://github.com/prometheus/docs/blob/master/content/docs/instrumenting/exposition_formats.md">Prometheus
 * exposition formats</a> for more details.
 *
 * @author Sergey Polovko
 */
final class PrometheusWriterImpl implements AutoCloseable {

    private final Writer w;
    private final StringBuilder tmpBuf = new StringBuilder(128);
    private final Map<String, MetricType> writtenTypes = new HashMap<>();

    PrometheusWriterImpl(Writer w) {
        this.w = w;
    }

    void writeType(MetricType type, String name) {
        if (writtenTypes.put(name, type) != null) {
            // type for this metric was already written
            return;
        }

        tmpBuf.setLength(0);
        tmpBuf.append("# TYPE ");
        appendWithMangling(tmpBuf, name);
        tmpBuf.append(' ');

        switch (type) {
            case DGAUGE:
            case IGAUGE:
                tmpBuf.append("gauge");
                break;
            case RATE:
            case COUNTER:
                tmpBuf.append("counter");
                break;
            case HIST:
            case HIST_RATE:
                tmpBuf.append("histogram");
                break;
            default:
                throw new IllegalStateException("unsupported metric type: " + type + ", name: " + name);
        }

        tmpBuf.append('\n');
        safeWrite(tmpBuf.toString());
    }

    void writeValue(String name, Labels labels, long tsMillis, double value) {
        tmpBuf.setLength(0);

        // (1) name
        appendWithMangling(tmpBuf, name);

        // (2) labels
        if (!labels.isEmpty()) {
            tmpBuf.append('{');
            appendLabels(tmpBuf, labels);
            tmpBuf.append('}');
        }

        // (3) value
        tmpBuf.append(' ').append(Doubles.toString(value));

        // (4) timestamp
        if (tsMillis != 0) {
            tmpBuf.append(' ').append(tsMillis);
        }

        tmpBuf.append('\n');
        safeWrite(tmpBuf.toString());
    }

    void writeHistogram(String name, Labels labels, long tsMillis, HistogramSnapshot hist) {
        long totalCount = 0;
        for (int i = 0, count = hist.count(); i < count; i++) {
            totalCount += hist.value(i);
            double upperBound = hist.upperBound(i);
            if (upperBound == Histograms.INF_BOUND) {
                upperBound = Double.POSITIVE_INFINITY;
            }
            writeHistogramBucket(name, labels, upperBound, tsMillis, (double) totalCount);
        }
        writeValue(name + "_count", labels, tsMillis, (double) totalCount);
    }

    // Special
    void writeDoubleHistogram(String name, Labels labels, long tsMillis, double[] bounds, double[] buckets) {
        long totalCount = 0;
        boolean hasInfBound = false;
        for (int i = 0, count = bounds.length; i < count; i++) {
            totalCount += buckets[i];
            double upperBound = bounds[i];
            if (upperBound == Histograms.INF_BOUND) {
                upperBound = Double.POSITIVE_INFINITY;
                hasInfBound = true;
            }
            writeHistogramBucket(name, labels, upperBound, tsMillis, (double) totalCount);
        }
        if (!hasInfBound) {
            writeHistogramBucket(name, labels, Double.POSITIVE_INFINITY, tsMillis, (double) totalCount);
        }
        writeValue(name + "_count", labels, tsMillis, (double) totalCount);
    }

    void writeHistogramBucket(String name, Labels labels, double upperBound, long tsMillis, double value) {
        tmpBuf.setLength(0);

        // (1) name
        appendWithMangling(tmpBuf, name);
        tmpBuf.append("_bucket");

        // (2) labels
        tmpBuf.append('{');
        if (!labels.isEmpty()) {
            appendLabels(tmpBuf, labels);
            tmpBuf.append(", ");
        }
        tmpBuf.append(PrometheusModel.BUCKET_LABEL);
        tmpBuf.append('=');
        appendLabelValue(tmpBuf, Doubles.toString(upperBound));
        tmpBuf.append('}');

        // (3) value
        tmpBuf.append(' ').append(Doubles.toString(value));

        // (4) timestamp
        if (tsMillis != 0) {
            tmpBuf.append(' ').append(tsMillis);
        }

        tmpBuf.append('\n');
        safeWrite(tmpBuf.toString());
    }

    private static void appendLabels(StringBuilder sb, Labels labels) {
        labels.forEach(label -> {
            appendWithMangling(sb, label.getKey());
            sb.append('=');
            appendLabelValue(sb, label.getValue());
            sb.append(", ");
        });
        sb.setLength(sb.length() - 2); // remove last ", "
    }

    /**
     * Translates invalid chars in value to '_'
     */
    static void appendWithMangling(StringBuilder sb, String name) {
        char ch = name.charAt(0);
        if (ch > 0x7f || !PrometheusModel.isValidMetricNameStart((byte) ch)) {
            sb.append('_');
        } else {
            sb.append(ch);
        }

        for (int i = 1, len = name.length(); i < len; i++) {
            ch = name.charAt(i);
            if (ch > 0x7f || !PrometheusModel.isValidMetricNameContinuation((byte) ch)) {
                sb.append('_');
            } else {
                sb.append(ch);
            }
        }
    }

    private static void appendLabelValue(StringBuilder sb, String value) {
        sb.append('"');
        for (int i = 0, len = value.length(); i < len; i++) {
            char ch = value.charAt(i);
            if (ch == '"') {
                sb.append("\\\"");
            } else if (ch == '\\') {
                sb.append("\\\\");
            } else if (ch == '\n') {
                sb.append("\\n");
            } else {
                sb.append(ch);
            }
        }
        sb.append('"');
    }

    private void safeWrite(String v) {
        try {
            w.write(v);
        } catch (IOException e) {
            throw new UncheckedIOException("cannot write: " + v, e);
        }
    }

    @Override
    public void close() {
        try {
            w.write('\n'); // last line always must be empty
            w.close();
        } catch (IOException e) {
            throw new UncheckedIOException("cannot close encoder", e);
        }
    }
}
