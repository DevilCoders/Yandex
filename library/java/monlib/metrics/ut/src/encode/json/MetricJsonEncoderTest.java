package ru.yandex.monlib.metrics.encode.json;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
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
public class MetricJsonEncoderTest {

    @Test
    public void emptyStream() throws Exception {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            e.onStreamEnd();
        });
        assertEquals("{}", result);
    }

    @Test
    public void withCommonTime() throws Exception {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            e.onCommonTime(1234567);
            e.onStreamEnd();
        });
        assertEquals("{\"ts\":1234}", result);
    }

    @Test
    public void withCommonLabels() throws Exception {
        String result1 = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onLabelsBegin(-1);
                e.onLabel("one", "1");
                e.onLabelsEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"labels\":{" +
                    "\"one\":\"1\"" +
                "}" +
            "}", result1);

        String result2 = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onLabelsBegin(-1);
                e.onLabel("one", "1");
                e.onLabel("two", "2");
                e.onLabelsEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"labels\":{" +
                    "\"one\":\"1\"," +
                    "\"two\":\"2\"" +
                "}" +
            "}", result2);
    }

    @Test
    public void withCommonTimeAndLabels() throws Exception {
        String result1 = encodeToString(e -> {
            e.onStreamBegin(-1);
            e.onCommonTime(1234567);
            {
                e.onLabelsBegin(-1);
                e.onLabel("one", "1");
                e.onLabelsEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"ts\":1234," +
                "\"labels\":{" +
                    "\"one\":\"1\"" +
                "}" +
            "}", result1);

        String result2 = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onLabelsBegin(-1);
                e.onLabel("one", "1");
                e.onLabelsEnd();
            }
            e.onCommonTime(1234567);
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"labels\":{" +
                    "\"one\":\"1\"" +
                "}," +
                "\"ts\":1234" +
            "}", result2);
    }

    @Test
    public void withOneMetric() {
        String result1 = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onDouble(123456, 3.14159);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"DGAUGE\"," +
                        "\"labels\":{" +
                            "\"one\":\"1\"" +
                        "}," +
                        "\"ts\":123," +
                        "\"value\":3.14159" +
                    "}" +
                "]" +
            "}", result1);

        String result2 = encodeToString(e -> {
            e.onStreamBegin(-1);
            e.onCommonTime(123456);
            {
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onDouble(123456, 3.14159);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"ts\":123," +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"DGAUGE\"," +
                        "\"labels\":{" +
                            "\"one\":\"1\"" +
                        "}," +
                        "\"ts\":123," +
                        "\"value\":3.14159" +
                    "}" +
                "]" +
            "}", result2);
    }

    @Test
    public void withOneHistogram() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onHistogram(123456, histogram(new double[]{10, 20, Histograms.INF_BOUND}, new long[]{2, 1, 5}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"HIST\"," +
                        "\"labels\":{\"one\":\"1\"}," +
                        "\"ts\":123," +
                        "\"hist\":{\"bounds\":[10.0,20.0],\"buckets\":[2,1],\"inf\":5}" +
                    "}" +
                "]" +
            "}", result);
    }

    @Test
    public void withHistogramWithoutInfBucket() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onHistogram(123456, histogram(new double[]{10, 20, 30}, new long[]{2, 1, 5}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"HIST\"," +
                        "\"labels\":{\"one\":\"1\"}," +
                        "\"ts\":123," +
                        "\"hist\":{\"bounds\":[10.0,20.0,30.0],\"buckets\":[2,1,5]}" +
                    "}" +
                "]" +
            "}", result);
    }


    @Test
    public void withOneHistogramRate() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.HIST_RATE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onHistogram(123456, histogram(new double[]{10, 20, Histograms.INF_BOUND}, new long[]{2, 0, 0}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"HIST_RATE\"," +
                        "\"labels\":{\"one\":\"1\"}," +
                        "\"ts\":123," +
                        "\"hist\":{\"bounds\":[10.0,20.0],\"buckets\":[2,0],\"inf\":0}" +
                    "}" +
                "]" +
            "}", result);
    }

    @Test
    public void withTwoMetric() {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onDouble(123456, 3.14159);
                e.onMetricEnd();
            }
            {
                e.onMetricBegin(MetricType.COUNTER);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("two", "2");
                    e.onLabel("three", "3");
                    e.onLabelsEnd();
                    e.onLong(654321, 42);
                }
                e.onMetricEnd();
            }
            {
                e.onMetricBegin(MetricType.IGAUGE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("five", "5");
                    e.onLabelsEnd();
                }
                e.onLong(789012, 57);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"DGAUGE\"," +
                        "\"labels\":{" +
                            "\"one\":\"1\"" +
                        "}," +
                        "\"ts\":123," +
                        "\"value\":3.14159" +
                    "}," +
                    "{" +
                        "\"kind\":\"COUNTER\"," +
                        "\"labels\":{" +
                            "\"two\":\"2\"," +
                            "\"three\":\"3\"" +
                        "}," +
                        "\"ts\":654," +
                        "\"value\":42" +
                    "}," +
                    "{" +
                        "\"kind\":\"IGAUGE\"," +
                        "\"labels\":{" +
                            "\"five\":\"5\"" +
                        "}," +
                        "\"ts\":789," +
                        "\"value\":57" +
                    "}" +
                "]" +
            "}", result);
    }

    @Test
    public void metricWithMultiplePoints() throws Exception {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.DGAUGE);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onDouble(123456, 3.14159);
                e.onDouble(654321, 2.71828);
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });
        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"DGAUGE\"," +
                        "\"labels\":{" +
                            "\"one\":\"1\"" +
                        "}," +
                        "\"timeseries\":[" +
                            "{" +
                                "\"ts\":123," +
                                "\"value\":3.14159" +
                            "}," +
                            "{" +
                                "\"ts\":654," +
                                "\"value\":2.71828" +
                            "}" +
                        "]" +
                    "}" +
                "]" +
            "}", result);
    }

    @Test
    public void metricWithMultipleHistogram() throws Exception {
        String result = encodeToString(e -> {
            e.onStreamBegin(-1);
            {
                e.onMetricBegin(MetricType.HIST);
                {
                    e.onLabelsBegin(-1);
                    e.onLabel("one", "1");
                    e.onLabelsEnd();
                }
                e.onHistogram(123456, histogram(new double[]{10, 20, Histograms.INF_BOUND}, new long[]{2, 1, 55}));
                e.onHistogram(654321, histogram(new double[]{10, 20, Histograms.INF_BOUND}, new long[]{50, 2, 0}));
                e.onMetricEnd();
            }
            e.onStreamEnd();
        });

        assertEquals(
            "{" +
                "\"sensors\":[" +
                    "{" +
                        "\"kind\":\"HIST\"," +
                        "\"labels\":{" +
                            "\"one\":\"1\"" +
                        "}," +
                        "\"timeseries\":[" +
                            "{" +
                                "\"ts\":123," +
                                "\"hist\":{\"bounds\":[10.0,20.0],\"buckets\":[2,1],\"inf\":55}" +
                            "}," +
                            "{" +
                                "\"ts\":654," +
                                "\"hist\":{\"bounds\":[10.0,20.0],\"buckets\":[50,2],\"inf\":0}" +
                            "}" +
                        "]" +
                    "}" +
                "]" +
            "}", result);
    }

    private static String encodeToString(Consumer<MetricJsonEncoder> consumer) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        try (MetricJsonEncoder e = new MetricJsonEncoder(out)) {
            consumer.accept(e);
        }
        return new String(out.toByteArray(), StandardCharsets.UTF_8);
    }

    private static HistogramSnapshot histogram(double[] bounds, long[] buckets) {
        return new ExplicitHistogramSnapshot(bounds, buckets);
    }
}
