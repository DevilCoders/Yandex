package ru.yandex.monlib.metrics.encode.text;

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
@SuppressWarnings("Duplicates")
public class MetricTextEncoderTest {

    @Test
    public void empty() throws Exception {
        String result = encodeToString(true, e -> {
            e.onStreamBegin(-1);
            e.onStreamEnd();
        });
        assertEquals("", result);
    }

    @Test
    public void commonPart() throws Exception {
        String result = encodeToString(true, e -> {
            e.onStreamBegin(-1);
            e.onCommonTime(Instant.parse("2017-01-02T03:04:05.006Z").toEpochMilli());
            {
                e.onLabelsBegin(3);
                e.onLabel("project", "solomon");
                e.onLabel("cluster", "man");
                e.onLabel("service", "stockpile");
                e.onLabelsEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "common time: 2017-01-02T03:04:05.006Z\n" +
            "common labels: {cluster='man', project='solomon', service='stockpile'}\n",
            result);
    }

    @Test
    public void gaugesDouble() throws Exception {
        Consumer<MetricTextEncoder> doEncode = e -> {
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
        };

        String result1 = encodeToString(false, doEncode);
        assertEquals(
            "   DGAUGE cpuUsage{}\n" +
            "   DGAUGE diskUsage{disk='sda1'} [1000.0]\n" +
            "   DGAUGE memoryUsage{dc='man', host='solomon-man-00'} [(1512216000, 1000.0)]\n" +
            "   DGAUGE bytesRx{dc='sas', host='solomon-sas-01'} [(1512216000, 2.0), (1512216005, 4.0), (1512216010, 8.0)]\n",
            result1);

        String result2 = encodeToString(true, doEncode);
        assertEquals(
            "   DGAUGE cpuUsage{}\n" +
            "   DGAUGE diskUsage{disk='sda1'} [1000.0]\n" +
            "   DGAUGE memoryUsage{dc='man', host='solomon-man-00'} [(2017-12-02T12:00:00Z, 1000.0)]\n" +
            "   DGAUGE bytesRx{dc='sas', host='solomon-sas-01'} [(2017-12-02T12:00:00Z, 2.0), (2017-12-02T12:00:05Z, 4.0), (2017-12-02T12:00:10Z, 8.0)]\n",
            result2);
    }

    @Test
    public void gaugesInt64() throws Exception {
        Consumer<MetricTextEncoder> doEncode = e -> {
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
        };

        String result1 = encodeToString(false, doEncode);
        assertEquals(
            "   IGAUGE cpuUsage{}\n" +
            "   IGAUGE diskUsage{disk='sda1'} [1000]\n" +
            "   IGAUGE memoryUsage{dc='man', host='solomon-man-00'} [(1512216000, 1000)]\n" +
            "   IGAUGE bytesRx{dc='sas', host='solomon-sas-01'} [(1512216000, 2), (1512216005, 4), (1512216010, 8)]\n",
            result1);

        String result2 = encodeToString(true, doEncode);
        assertEquals(
            "   IGAUGE cpuUsage{}\n" +
            "   IGAUGE diskUsage{disk='sda1'} [1000]\n" +
            "   IGAUGE memoryUsage{dc='man', host='solomon-man-00'} [(2017-12-02T12:00:00Z, 1000)]\n" +
            "   IGAUGE bytesRx{dc='sas', host='solomon-sas-01'} [(2017-12-02T12:00:00Z, 2), (2017-12-02T12:00:05Z, 4), (2017-12-02T12:00:10Z, 8)]\n",
            result2);
    }

    @Test
    public void counters() throws Exception {
        Consumer<MetricTextEncoder> doEncode = e -> {
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
        };

        String result1 = encodeToString(false, doEncode);
        assertEquals(
            "  COUNTER cpuUsage{}\n" +
            "  COUNTER diskUsage{disk='sda1'} [1000]\n" +
            "  COUNTER memoryUsage{dc='man', host='solomon-man-00'} [(1512216000, 1000)]\n" +
            "  COUNTER bytesRx{dc='sas', host='solomon-sas-01'} [(1512216000, 2), (1512216005, 4), (1512216010, 8)]\n",
            result1);

        String result2 = encodeToString(true, doEncode);
        assertEquals(
            "  COUNTER cpuUsage{}\n" +
            "  COUNTER diskUsage{disk='sda1'} [1000]\n" +
            "  COUNTER memoryUsage{dc='man', host='solomon-man-00'} [(2017-12-02T12:00:00Z, 1000)]\n" +
            "  COUNTER bytesRx{dc='sas', host='solomon-sas-01'} [(2017-12-02T12:00:00Z, 2), (2017-12-02T12:00:05Z, 4), (2017-12-02T12:00:10Z, 8)]\n",
            result2);
    }

    @Test
    public void histogram() {
        Consumer<MetricTextEncoder> doEncode = e -> {
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
                e.onHistogram(0,
                        histogram(new double[]{10L, 20L, Histograms.INF_BOUND}, new long[]{1L, 4L, 0L}));
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
                e.onHistogram(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(),
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
                e.onHistogram(Instant.parse("2017-12-02T12:00:00Z").toEpochMilli(),
                        histogram(bounds, new long[]{10L, 0L, 0L}));
                e.onHistogram(Instant.parse("2017-12-02T12:00:05Z").toEpochMilli(),
                        histogram(bounds, new long[]{10L, 2L, 0L}));
                e.onHistogram(Instant.parse("2017-12-02T12:00:10Z").toEpochMilli(),
                        histogram(bounds, new long[]{10L, 2L, 5L}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        };

        String result1 = encodeToString(false, doEncode);
        assertEquals(
                "     HIST cpuUsage{}\n" +
                        "     HIST inboundBytesPerSec{client='mbus'} [{10: 1, 20: 4, inf: 0}]\n" +
                        "     HIST inboundBytesPerSec{client='grpc'} [{10: 1, 20: 4, 30: 0}]\n" +
                        "HIST_RATE outboundBytesPerSec{client='grps'} [(1512216000, {100: 1, 200: 0, inf: 0})]\n" +
                        "     HIST bytesRx{dc='sas', host='solomon-sas-01'} [(1512216000, {100: 10, 200: 0, inf: 0}), (1512216005, {100: 10, 200: 2, inf: 0}), (1512216010, {100: 10, 200: 2, inf: 5})]\n",
                result1);

        String result2 = encodeToString(true, doEncode);
        assertEquals(
                "     HIST cpuUsage{}\n" +
                        "     HIST inboundBytesPerSec{client='mbus'} [{10: 1, 20: 4, inf: 0}]\n" +
                        "     HIST inboundBytesPerSec{client='grpc'} [{10: 1, 20: 4, 30: 0}]\n" +
                        "HIST_RATE outboundBytesPerSec{client='grps'} [(2017-12-02T12:00:00Z, {100: 1, 200: 0, inf: 0})]\n" +
                        "     HIST bytesRx{dc='sas', host='solomon-sas-01'} [(2017-12-02T12:00:00Z, {100: 10, 200: 0, inf: 0}), (2017-12-02T12:00:05Z, {100: 10, 200: 2, inf: 0}), (2017-12-02T12:00:10Z, {100: 10, 200: 2, inf: 5})]\n",
                result2);
    }

    private static HistogramSnapshot histogram(double[] bounds, long[] buckets) {
        return new ExplicitHistogramSnapshot(bounds, buckets);
    }

    private static String encodeToString(boolean humanReadableTime, Consumer<MetricTextEncoder> consumer) {
        StringWriter out = new StringWriter(256);
        try (MetricTextEncoder e = new MetricTextEncoder(out, humanReadableTime)) {
            consumer.accept(e);
        }
        return out.toString();
    }
}
