package ru.yandex.monlib.metrics.encode.prometheus;

import java.nio.charset.StandardCharsets;

import org.junit.Assert;
import org.junit.Test;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.MetricsData;
import ru.yandex.monlib.metrics.MetricsData.Metric;
import ru.yandex.monlib.metrics.TestMetricConsumer;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.junit.Assert.assertEquals;


/**
 * @author Sergey Polovko
 */
public class MetricPrometheusDecoderTest {

    @Test
    public void empty() {
        {
            MetricsData metrics = decode("");
            Assert.assertEquals(0, metrics.size());
        }
        {
            MetricsData metrics = decode("\n");
            Assert.assertEquals(0, metrics.size());
        }
        {
            MetricsData metrics = decode("\n\n\n");
            Assert.assertEquals(0, metrics.size());
        }
        {
            MetricsData metrics = decode("\t\n\t\n");
            Assert.assertEquals(0, metrics.size());
        }
        {
            MetricsData metrics = decode("\n \n\t\n \t ");
            Assert.assertEquals(0, metrics.size());
        }
    }

    @Test
    public void minimal() {
        MetricsData metrics = decode(
            "minimal_metric 1.234\n" +
            "another_metric -3e3 103948\n" +
            "# Even that:\n" +
            "no_labels{} 3\n" +
            "# HELP line for non-existing metric will be ignored.\n");

        Assert.assertEquals(3, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "minimal_metric"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 1.234), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "another_metric"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(103948, -3000), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "no_labels"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 3), metric.timeSeries);
        }
    }

    @Test
    public void counter() {
        MetricsData metrics = decode(
            "# A normal comment.\n" +
            "#\n" +
            "# TYPE name counter\n" +
            "name{labelname=\"val1\",basename=\"basevalue\"} NaN\n" +
            "name {labelname=\"val2\",basename=\"basevalue\"} 2.3 1234567890\n" +
            "# HELP name two-line\\n doc  str\\\\ing\n");

        Assert.assertEquals(2, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "name", "labelname", "val1", "basename", "basevalue"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "name", "labelname", "val2", "basename", "basevalue"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(1234567890, 2), metric.timeSeries);
        }
    }

    @Test
    public void gauge() {
        MetricsData metrics = decode(
            "# A normal comment.\n" +
            "#\n" +
            " # HELP  name2  \tdoc str\"ing 2\n" +
            "  #    TYPE    name2 gauge\n" +
            "name2{labelname=\"val2\"\t,basename   =   \"basevalue2\"\t\t} +Inf 54321\n" +
            "name2{ labelname = \"val1\" , }-Inf\n");

        Assert.assertEquals(2, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "name2", "labelname", "val2", "basename", "basevalue2"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(54321, Double.POSITIVE_INFINITY), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "name2", "labelname", "val1"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, Double.NEGATIVE_INFINITY), metric.timeSeries);
        }
    }

    @Test
    public void summary() {
        // The evil summary, mixed with other types and funny comments.
        MetricsData metrics = decode(
            "# HELP \n" +
            "# TYPE my_summary summary\n" +
            "my_summary{n1=\"val1\",quantile=\"0.5\"} 110\n" +
            "my_summary{n1=\"val1\",quantile=\"0.9\"} 140 1\n" +
            "my_summary_count{n1=\"val1\"} 42\n" +
            "my_summary_sum{n1=\"val1\"} 08 15\n" +
            "# some\n" +
            "# funny comments\n" +
            "# HELP\n" +
            "# HELP my_summary\n" +
            "# HELP my_summary \n");

        Assert.assertEquals(4, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "my_summary_count", "n1", "val1"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 42), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "my_summary_sum", "n1", "val1"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(15, 8), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "my_summary", "n1", "val1", "quantile", "0.5"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 110), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "my_summary", "n1", "val1", "quantile", "0.9"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(1, 140), metric.timeSeries);
        }
    }

    @Test
    public void histogram() {
        MetricsData metrics = decode(
            "# HELP request_duration_microseconds The response latency.\n" +
            "# TYPE request_duration_microseconds histogram\n" +
            "request_duration_microseconds_bucket{le=\"100\"} 123\n" +
            "request_duration_microseconds_bucket{le=\"120\"} 412\n" +
            "request_duration_microseconds_bucket{le=\"144\"} 592\n" +
            "request_duration_microseconds_bucket{le=\"172.8\"} 1524\n" +
            "request_duration_microseconds_bucket{le=\"+Inf\"} 2693\n" +
            "request_duration_microseconds_sum 1.7560473e+06\n" +
            "request_duration_microseconds_count 2693\n");

        Assert.assertEquals(3, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "request_duration_microseconds_count"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 2693), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "request_duration_microseconds_sum"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 1756047.3), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "request_duration_microseconds"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            ExplicitHistogramSnapshot histogram = new ExplicitHistogramSnapshot(
                new double[]{ 100, 120, 144, 172.8, Histograms.INF_BOUND },
                new long[]{ 123, 289, 180, 932, 1169 });
            assertEquals(TimeSeries.newHistogram(0, histogram), metric.timeSeries);
        }
    }

    @Test
    public void histogramWithLabels() {
        MetricsData metrics = decode(
            "# A histogram, which has a pretty complex representation in the text format:\n" +
            "# HELP http_request_duration_seconds A histogram of the request duration.\n" +
            "# TYPE http_request_duration_seconds histogram\n" +
            "http_request_duration_seconds_bucket{le=\"0.05\", method=\"POST\"} 24054\n" +
            "http_request_duration_seconds_bucket{method=\"POST\", le=\"0.1\"} 33444\n" +
            "http_request_duration_seconds_bucket{le=\"0.2\", method=\"POST\", } 100392\n" +
            "http_request_duration_seconds_bucket{le=\"0.5\",method=\"POST\",} 129389\n" +
            "http_request_duration_seconds_bucket{ method=\"POST\", le=\"1\", } 133988\n" +
            "http_request_duration_seconds_bucket{ le=\"+Inf\", method=\"POST\", } 144320\n" +
            "http_request_duration_seconds_sum{method=\"POST\"} 53423\n" +
            "http_request_duration_seconds_count{ method=\"POST\", } 144320\n");

        Assert.assertEquals(3, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_request_duration_seconds_count", "method", "POST"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 144320), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_request_duration_seconds_sum", "method", "POST"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 53423.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_request_duration_seconds", "method", "POST"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            ExplicitHistogramSnapshot histogram = new ExplicitHistogramSnapshot(
                new double[]{ 0.05, 0.1, 0.2, 0.5, 1, Histograms.INF_BOUND },
                new long[]{ 24054, 9390, 66948, 28997, 4599, 10332 });
            assertEquals(TimeSeries.newHistogram(0, histogram), metric.timeSeries);
        }
    }

    @Test
    public void multipleHistograms() {
        MetricsData metrics = decode(
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
            "outboundBytesPerSec_count{client=\"grps\"} 1.0 1512216000000\n");

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "inboundBytesPerSec_count", "client", "grpc"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 5), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "inboundBytesPerSec", "client", "grpc"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            ExplicitHistogramSnapshot histogram = new ExplicitHistogramSnapshot(
                new double[]{ 10, 20, 30 },
                new long[]{ 1, 4, 0 });
            assertEquals(TimeSeries.newHistogram(0, histogram), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "inboundBytesPerSec_count", "client", "mbus"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 5), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "inboundBytesPerSec", "client", "mbus"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            ExplicitHistogramSnapshot histogram = new ExplicitHistogramSnapshot(
                new double[]{ 10, 20, Histograms.INF_BOUND },
                new long[]{ 1, 4, 0 });
            assertEquals(TimeSeries.newHistogram(0, histogram), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "outboundBytesPerSec_count", "client", "grps"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(1512216000000L, 1), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "outboundBytesPerSec", "client", "grps"));
            assertEquals(MetricType.HIST_RATE, metric.type);
            ExplicitHistogramSnapshot histogram = new ExplicitHistogramSnapshot(
                new double[]{ 100, 200, Histograms.INF_BOUND },
                new long[]{ 1, 0, 0 });
            assertEquals(TimeSeries.newHistogram(1512216000000L, histogram), metric.timeSeries);
        }
    }

    @Test
    public void mixedTypes() {
        MetricsData metrics = decode(
            "# HELP http_requests_total The total number of HTTP requests.\n" +
            "# TYPE http_requests_total counter\n" +
            "http_requests_total { } 1027 1395066363000\n" +
            "http_requests_total{method=\"post\",code=\"200\"} 1027 1395066363000\n" +
            "http_requests_total{method=\"post\",code=\"400\"}    3 1395066363000\n" +
            "\n" +
            "\n" +
            "# Minimalistic line:\n" +
            "metric_without_timestamp_and_labels 12.47\n" +
            "\n" +
            "# HELP rpc_duration_seconds A summary of the RPC duration in seconds.\n" +
            "# TYPE rpc_duration_seconds summary\n" +
            "rpc_duration_seconds{quantile=\"0.01\"} 3102\n" +
            "rpc_duration_seconds{quantile=\"0.05\"} 3272\n" +
            "rpc_duration_seconds{quantile=\"0.5\"} 4773\n" +
            "rpc_duration_seconds{quantile=\"0.9\"} 9001\n" +
            "rpc_duration_seconds{quantile=\"0.99\"} 76656\n" +
            "rpc_duration_seconds_sum 1.7560473e+07\n" +
            "rpc_duration_seconds_count 2693\n" +
            "\n" +
            "# Another mMinimalistic line:\n" +
            "metric_with_timestamp 12.47 1234567890\n");

        Assert.assertEquals(12, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_requests_total"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(1395066363000L, 1027), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_requests_total", "code", "200", "method", "post"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(1395066363000L, 1027), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "http_requests_total", "code", "400", "method", "post"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(1395066363000L, 3), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "metric_without_timestamp_and_labels"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 12.47), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("quantile", "0.01", "sensor", "rpc_duration_seconds"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 3102.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("quantile", "0.05", "sensor", "rpc_duration_seconds"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 3272.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("quantile", "0.5", "sensor", "rpc_duration_seconds"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 4773.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("quantile", "0.9", "sensor", "rpc_duration_seconds"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 9001.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("quantile", "0.99", "sensor", "rpc_duration_seconds"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 76656.0), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "rpc_duration_seconds_sum"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 1.7560473E7), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "rpc_duration_seconds_count"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 2693), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "metric_with_timestamp"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(1234567890L, 12.47), metric.timeSeries);
        }
    }

    private static MetricsData decode(String data) {
        MetricPrometheusDecoder decoder = new MetricPrometheusDecoder();
        TestMetricConsumer consumer = new TestMetricConsumer();
        decoder.decode(data.getBytes(StandardCharsets.UTF_8), consumer);
        return consumer.getMetricsData();
    }
}
