package ru.yandex.monlib.metrics.encode.spack;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.time.Instant;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.MetricsData;
import ru.yandex.monlib.metrics.TestMetricConsumer;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.junit.Assert.assertEquals;


/**
 * @author Sergey Polovko
 */
public class MetricSpackDecoderTest {

    @Test
    public void decodeNonCompressed() throws Exception {
        testDecoding("test.sp");
    }

    @Test
    public void decodeZlib() throws Exception {
        testDecoding("test.sp.zlib");
    }

    @Test
    public void decodeLz4() throws Exception {
        testDecoding("test.sp.lz4");
    }

    @Test
    public void decodeZstd() throws Exception {
        testDecoding("test.sp.zstd");
    }

    private static void testDecoding(String resourceName) {
        MetricsData metrics = decode(loadResource(resourceName));
        System.out.println(metrics);

        Instant expectedCommonTs = Instant.parse("2017-08-27T12:34:56Z");
        assertEquals(expectedCommonTs.toEpochMilli(), metrics.getCommonTsMillis());
        assertEquals(
            Labels.of("project", "solomon", "cluster", "man", "service", "stockpile"),
            metrics.getCommonLabels());

        assertEquals(5, metrics.size());
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "QueueSize", "export", "Oxygen"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(1509885296000L, 3.14159), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "Memory"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 10.0), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "UserTime"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 1), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "Writes"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(
                TimeSeries.newDouble(1503923531000L, -10.0)
                    .addDouble(1503923187000L, 20.0),
                metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "SystemTime"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, -1), metric.timeSeries);
        }
    }

    private static MetricsData decode(ByteBuffer buffer) {
        MetricSpackDecoder decoder = new MetricSpackDecoder();
        TestMetricConsumer consumer = new TestMetricConsumer();
        decoder.decode(buffer, consumer);
        return consumer.getMetricsData();
    }

    private static ByteBuffer loadResource(String name) {
        try (InputStream in = MetricSpackDecoderTest.class.getResourceAsStream(name)) {
            ByteArrayOutputStream out = new ByteArrayOutputStream(8 << 10);
            for (int b; (b = in.read()) != -1;) {
                out.write(b);
            }
            return ByteBuffer.wrap(out.toByteArray());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
