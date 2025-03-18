package ru.yandex.monlib.metrics.encode.prometheus;

import java.io.StringWriter;
import java.time.Instant;
import java.util.function.Consumer;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;

import static org.junit.Assert.assertEquals;


/**
 * @author Sergey Polovko
 */
public class MetricPrometheusEncoderTest {

    @Test
    public void empty() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            e.onStreamEnd();
        });
        assertEquals("\n", result);
    }

    @Test
    public void gaugesDouble() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            { // no values
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "cpuUsage");
                    e.onLabelsEnd();
                }
                e.onMetricEnd();
            }
            { // one value no ts
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "diskUsage");
                    e.onLabel("disk", "sda1");
                    e.onLabelsEnd();
                }
                e.onDouble(0, 1000);
                e.onMetricEnd();
            }
            { // one value with ts
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "memoryUsage");
                    e.onLabel("host", "solomon-man-00");
                    e.onLabel("dc", "man");
                    e.onLabelsEnd();
                }
                e.onDouble(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 1000);
                e.onMetricEnd();
            }
            { // many values
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "bytesRx");
                    e.onLabel("host", "solomon-sas-01");
                    e.onLabel("dc", "sas");
                    e.onLabelsEnd();
                }
                e.onDouble(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 2);
                e.onDouble(Instant.parse("2017-12-02T12:00:05Z").toEpochMilli(), 4);
                e.onDouble(Instant.parse("2017-12-02T12:00:10Z").toEpochMilli(), 8);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "# TYPE diskUsage gauge\n" +
            "diskUsage{disk=\"sda1\"} 1000.0\n" +
            "# TYPE memoryUsage gauge\n" +
            "memoryUsage{dc=\"man\", host=\"solomon-man-00\"} 1000.0 1512216000000\n" +
            "# TYPE bytesRx gauge\n" +
            "bytesRx{dc=\"sas\", host=\"solomon-sas-01\"} 8.0 1512216010000\n" +
            "\n",
            result);
    }

    @Test
    public void gaugesInt64() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            { // no values
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "cpuUsage");
                    e.onLabelsEnd();
                }
                e.onMetricEnd();
            }
            { // one value no ts
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "diskUsage");
                    e.onLabel("disk", "sda1");
                    e.onLabelsEnd();
                }
                e.onLong(0, 1000);
                e.onMetricEnd();
            }
            { // one value with ts
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "memoryUsage");
                    e.onLabel("host", "solomon-man-00");
                    e.onLabel("dc", "man");
                    e.onLabelsEnd();
                }
                e.onLong(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 1000);
                e.onMetricEnd();
            }
            { // many values
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "bytesRx");
                    e.onLabel("host", "solomon-sas-01");
                    e.onLabel("dc", "sas");
                    e.onLabelsEnd();
                }
                e.onLong(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 2);
                e.onLong(Instant.parse("2017-12-02T12:00:05Z").toEpochMilli(), 4);
                e.onLong(Instant.parse("2017-12-02T12:00:10Z").toEpochMilli(), 8);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "# TYPE diskUsage gauge\n" +
            "diskUsage{disk=\"sda1\"} 1000.0\n" +
            "# TYPE memoryUsage gauge\n" +
            "memoryUsage{dc=\"man\", host=\"solomon-man-00\"} 1000.0 1512216000000\n" +
            "# TYPE bytesRx gauge\n" +
            "bytesRx{dc=\"sas\", host=\"solomon-sas-01\"} 8.0 1512216010000\n" +
            "\n",
            result);
    }

    @Test
    public void counters() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            { // no values
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "cpuUsage");
                    e.onLabelsEnd();
                }
                e.onMetricEnd();
            }
            { // one value no ts
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "diskUsage");
                    e.onLabel("disk", "sda1");
                    e.onLabelsEnd();
                }
                e.onLong(0, 1000);
                e.onMetricEnd();
            }
            { // one value with ts
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "memoryUsage");
                    e.onLabel("host", "solomon-man-00");
                    e.onLabel("dc", "man");
                    e.onLabelsEnd();
                }
                e.onLong(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 1000);
                e.onMetricEnd();
            }
            { // many values
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "bytesRx");
                    e.onLabel("host", "solomon-sas-01");
                    e.onLabel("dc", "sas");
                    e.onLabelsEnd();
                }
                e.onLong(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(), 2);
                e.onLong(Instant.parse("2017-12-02T12:00:05Z").toEpochMilli(), 4);
                e.onLong(Instant.parse("2017-12-02T12:00:10Z").toEpochMilli(), 8);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "# TYPE diskUsage counter\n" +
            "diskUsage{disk=\"sda1\"} 1000.0\n" +
            "# TYPE memoryUsage counter\n" +
            "memoryUsage{dc=\"man\", host=\"solomon-man-00\"} 1000.0 1512216000000\n" +
            "# TYPE bytesRx counter\n" +
            "bytesRx{dc=\"sas\", host=\"solomon-sas-01\"} 8.0 1512216010000\n" +
            "\n",
            result);
    }

    @Test
    public void histogram() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            { // no values histogram
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "cpuUsage");
                    e.onLabelsEnd();
                }
                e.onMetricEnd();
            }
            { // one value no ts
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "inboundBytesPerSec");
                    e.onLabel("client", "mbus");
                    e.onLabelsEnd();
                }
                e.onHistogram(0, histogram(new double[]{10L, 20L, Histograms.INF_BOUND}, new long[]{1L, 4L, 0L}));
                e.onMetricEnd();
            }
            { // one value no ts no +inf bucket
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "inboundBytesPerSec");
                    e.onLabel("client", "grpc");
                    e.onLabelsEnd();
                }
                e.onHistogram(0,
                    histogram(new double[]{10, 20, 30}, new long[]{1, 4, 0}));
                e.onMetricEnd();
            }
            { // one value with ts
                e.onMetricBegin(MetricType.HIST_RATE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "outboundBytesPerSec");
                    e.onLabel("client", "grps");
                    e.onLabelsEnd();
                }
                e.onHistogram(
                    Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(),
                    histogram(new double[]{100, 200, Histograms.INF_BOUND}, new long[]{1L, 0L, 0L}));
                e.onMetricEnd();
            }
            { // many values
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "bytesRx");
                    e.onLabel("host", "solomon-sas-01");
                    e.onLabel("dc", "sas");
                    e.onLabelsEnd();
                }
                double[] bounds = {100, 200, Histograms.INF_BOUND};
                e.onHistogram(
                    Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(),
                    histogram(bounds, new long[]{10L, 0L, 0L}));
                e.onHistogram(
                    Instant.parse("2017-12-02T12:00:05Z").toEpochMilli(),
                    histogram(bounds, new long[]{10L, 2L, 0L}));
                e.onHistogram(
                    Instant.parse("2017-12-02T12:00:10Z").toEpochMilli(),
                    histogram(bounds, new long[]{10L, 2L, 5L}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "# TYPE inboundBytesPerSec histogram\n" +
            "inboundBytesPerSec_bucket{client=\"mbus\", le=\"10.0\"} 1.0\n" +
            "inboundBytesPerSec_bucket{client=\"mbus\", le=\"20.0\"} 5.0\n" +
            "inboundBytesPerSec_bucket{client=\"mbus\", le=\"+Inf\"} 5.0\n" +
            "inboundBytesPerSec_count{client=\"mbus\"} 5.0\n" +
            "inboundBytesPerSec_bucket{client=\"grpc\", le=\"10.0\"} 1.0\n" +
            "inboundBytesPerSec_bucket{client=\"grpc\", le=\"20.0\"} 5.0\n" +
            "inboundBytesPerSec_bucket{client=\"grpc\", le=\"30.0\"} 5.0\n" +
            "inboundBytesPerSec_count{client=\"grpc\"} 5.0\n" +
            "# TYPE outboundBytesPerSec histogram\n" +
            "outboundBytesPerSec_bucket{client=\"grps\", le=\"100.0\"} 1.0 1512216000000\n" +
            "outboundBytesPerSec_bucket{client=\"grps\", le=\"200.0\"} 1.0 1512216000000\n" +
            "outboundBytesPerSec_bucket{client=\"grps\", le=\"+Inf\"} 1.0 1512216000000\n" +
            "outboundBytesPerSec_count{client=\"grps\"} 1.0 1512216000000\n" +
            "# TYPE bytesRx histogram\n" +
            "bytesRx_bucket{dc=\"sas\", host=\"solomon-sas-01\", le=\"100.0\"} 10.0 1512216010000\n" +
            "bytesRx_bucket{dc=\"sas\", host=\"solomon-sas-01\", le=\"200.0\"} 12.0 1512216010000\n" +
            "bytesRx_bucket{dc=\"sas\", host=\"solomon-sas-01\", le=\"+Inf\"} 17.0 1512216010000\n" +
            "bytesRx_count{dc=\"sas\", host=\"solomon-sas-01\"} 17.0 1512216010000\n" +
            "\n",
            result);
    }

    private static HistogramSnapshot histogram(double[] bounds, long[] buckets) {
        return new ExplicitHistogramSnapshot(bounds, buckets);
    }

    private static String encodeToString(Consumer<MetricPrometheusEncoder> consumer) {
        StringWriter out = new StringWriter(256);
        try (MetricPrometheusEncoder e = new MetricPrometheusEncoder(out)) {
            consumer.accept(e);
        }
        return out.toString();
    }
}
