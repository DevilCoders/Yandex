package ru.yandex.monlib.metrics.encode.prometheus;

import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;

import javax.annotation.ParametersAreNonnullByDefault;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.labels.Labels;

/**
 * @author Oleg Baryshnikov
 */
@ParametersAreNonnullByDefault
public class ExtendedPrometheusWriter implements AutoCloseable {
    private final PrometheusWriterImpl writer;

    public ExtendedPrometheusWriter(OutputStream out) {
        this.writer = new PrometheusWriterImpl(new OutputStreamWriter(out));
    }

    public ExtendedPrometheusWriter(Writer writer) {
        this.writer = new PrometheusWriterImpl(writer);
    }

    public void writeGauge(String name, Labels labels, double value) {
        writer.writeType(MetricType.DGAUGE, name);
        writer.writeValue(name, labels, 0, value);
    }

    public void writeCounter(String name, Labels labels, double value) {
        writer.writeType(MetricType.COUNTER, name);
        writer.writeValue(name, labels, 0, value);
    }

    public void writeHistogram(String name, Labels labels, double[] bounds, double[] buckets) {
        if (labels.hasKey(PrometheusModel.BUCKET_LABEL)) {
            String msg = String.format(
                    "labels %s of metric %s have reserved label '%s'",
                    labels, name, PrometheusModel.BUCKET_LABEL);
            throw new IllegalStateException(msg);
        }
        writer.writeType(MetricType.HIST, name);
        writer.writeDoubleHistogram(name, labels, 0, bounds, buckets);
    }

    @Override
    public void close() {
        writer.close();
    }
}
