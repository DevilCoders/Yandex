package ru.yandex.monlib.metrics.encode.spack;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.MetricsData;
import ru.yandex.monlib.metrics.TestMetricConsumer;
import ru.yandex.monlib.metrics.encode.spack.format.CompressionAlg;
import ru.yandex.monlib.metrics.encode.spack.format.SpackHeader;
import ru.yandex.monlib.metrics.encode.spack.format.TimePrecision;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.junit.Assert.assertEquals;


/**
 * @author Sergey Polovko
 */
public class MetricSpackEncoderTest {

    @Test
    public void emptyStream() throws Exception {
        ByteBuffer result = encodeToByteBuffer(TimePrecision.SECONDS, CompressionAlg.NONE, e -> {
            e.onStreamBegin(-1);
            e.onStreamEnd();
        });
        result.order(ByteOrder.LITTLE_ENDIAN);

        SpackHeader header = SpackHeader.readFrom(result);
        assertEquals(0, header.getMetricCount());
        assertEquals(0, header.getPointCount());
    }

    @Test
    public void encodeDecodeSeconds() throws Exception {
        doEncodeDecode(TimePrecision.SECONDS, CompressionAlg.NONE);
    }

    @Test
    public void encodeDecodeMillis() throws Exception {
        doEncodeDecode(TimePrecision.MILLIS, CompressionAlg.NONE);
    }

    @Test
    public void encodeDecodeZlib() throws Exception {
        doEncodeDecode(TimePrecision.SECONDS, CompressionAlg.ZLIB);
    }

    @Test
    public void encodeDecodeLz4() throws Exception {
        doEncodeDecode(TimePrecision.SECONDS, CompressionAlg.LZ4);
    }

    @Test
    public void encodeDecodeZstd() throws Exception {
        doEncodeDecode(TimePrecision.SECONDS, CompressionAlg.ZSTD);
    }

    private void doEncodeDecode(TimePrecision timePrecision, CompressionAlg compressionAlg) throws Exception {
        final long nowMillis = System.currentTimeMillis();
        final long alignedNowMillis = (timePrecision == TimePrecision.SECONDS)
            ? TimeUnit.SECONDS.toMillis(TimeUnit.MILLISECONDS.toSeconds(nowMillis))
            : nowMillis;

        ByteBuffer data = encodeToByteBuffer(timePrecision, compressionAlg, e -> {
            e.onStreamBegin(1);
            // common time
            e.onCommonTime(nowMillis);
            { // common labels
                e.onLabelsBegin(3);
                e.onLabel("project", "solomon");
                e.onLabel("cluster", "production");
                e.onLabel("service", "stockpile");
                e.onLabelsEnd();
            }
            { // metric #1 (no values)
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q1");
                    e.onLabelsEnd();
                }
                e.onMetricEnd();
            }
            { // metric #2 (one value without ts)
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q2");
                    e.onLabelsEnd();
                }
                e.onLong(0, 42);
                e.onMetricEnd();
            }
            { // metric #3 (one value with ts)
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q3");
                    e.onLabelsEnd();
                }
                e.onDouble(nowMillis + 5_000, 3.14159);
                e.onMetricEnd();
            }
            { // metric #4 (many values with ts)
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q4");
                    e.onLabelsEnd();
                }
                e.onLong(nowMillis + 5_000, 43);
                e.onLong(nowMillis + 10_000, 44);
                e.onLong(nowMillis + 15_000, 45);
                e.onMetricEnd();
            }
            { // metric #5 (one value with ts)
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q5");
                    e.onLabelsEnd();
                }
                e.onLong(nowMillis, 46);
                e.onMetricEnd();
            }
            { // metric #6 (one histogram)
                e.onMetricBegin(MetricType.HIST_RATE);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q6");
                    e.onLabelsEnd();
                }
                e.onHistogram(nowMillis,
                        Histograms.explicit(10, 20, 30)
                                .collect(12, 100)
                                .snapshot());
                e.onMetricEnd();
            }
            { // metric #7 (many histogram)
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q7");
                    e.onLabelsEnd();
                }
                e.onHistogram(nowMillis + 5_000,
                        Histograms.explicit(10, 20, 30)
                                .collect(3, 13)
                                .snapshot());
                e.onHistogram(nowMillis + 10_000,
                        Histograms.explicit(10, 20, 30)
                                .collect(24, 42)
                                .collect(12, 1)
                                .snapshot());
                e.onMetricEnd();
            }
            { // metric #8 (one histogram without +inf bucket)
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(1);
                    e.onLabel("sensor", "q8");
                    e.onLabelsEnd();
                }
                e.onHistogram(0, new ExplicitHistogramSnapshot(
                        new double[]{10, 20, 30},
                        new long[]{1,  2,  3}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        {
            data.mark();
            SpackHeader header = SpackHeader.readFrom(data);
            assertEquals(timePrecision, header.getTimePrecision());
            assertEquals(compressionAlg, header.getCompressionAlg());
            assertEquals(8, header.getMetricCount());
            assertEquals(10, header.getPointCount());
            data.reset();
        }

        MetricsData metrics = decode(data);
        assertEquals(8, metrics.size());
        assertEquals(alignedNowMillis, metrics.getCommonTsMillis());
        assertEquals(
            Labels.of("project", "solomon", "cluster", "production", "service", "stockpile"),
            metrics.getCommonLabels());
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q1"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.empty(), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q2"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 42), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q3"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(alignedNowMillis + 5_000, 3.14159), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q4"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(
                TimeSeries.newLong(3)
                    .addLong(alignedNowMillis +  5_000, 43)
                    .addLong(alignedNowMillis + 10_000, 44)
                    .addLong(alignedNowMillis + 15_000, 45),
                metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q5"));
            assertEquals(MetricType.IGAUGE, metric.type);
            assertEquals(TimeSeries.newLong(alignedNowMillis, 46), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q6"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            assertEquals(TimeSeries.newHistogram(alignedNowMillis, Histograms.explicit(10, 20, 30)
                    .collect(12, 100)
                    .snapshot()), metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q7"));
            assertEquals(MetricType.HIST, metric.type);
            assertEquals(
                    TimeSeries.newHistogram(2)
                            .addHistogram(alignedNowMillis +  5_000,
                                    Histograms.explicit(10, 20, 30)
                                            .collect(3, 13)
                                            .snapshot())
                            .addHistogram(alignedNowMillis + 10_000,
                                    Histograms.explicit(10, 20, 30)
                                            .collect(24, 42)
                                            .collect(12, 1)
                                            .snapshot()),
                    metric.timeSeries);
        }
        {
            MetricsData.Metric metric = metrics.getMetric(Labels.of("sensor", "q8"));
            assertEquals(MetricType.HIST, metric.type);
            assertEquals(TimeSeries.newHistogram(0, new ExplicitHistogramSnapshot(
                    new double[]{10, 20, 30},
                    new long[]{1,  2,  3})), metric.timeSeries);
        }
    }

    private static ByteBuffer encodeToByteBuffer(
        TimePrecision timePrecision,
        CompressionAlg compressionAlg,
        Consumer<MetricSpackEncoder> consumer)
    {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try (MetricSpackEncoder e = new MetricSpackEncoder(timePrecision, compressionAlg, out)) {
            consumer.accept(e);
        }
        return ByteBuffer.wrap(out.toByteArray());
    }

    private static MetricsData decode(ByteBuffer buffer) {
        MetricSpackDecoder decoder = new MetricSpackDecoder();
        TestMetricConsumer consumer = new TestMetricConsumer();
        decoder.decode(buffer, consumer);
        return consumer.getMetricsData();
    }
}
