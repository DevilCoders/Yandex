package ru.yandex.monlib.metrics;

import java.util.Arrays;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricsData.Metric;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Counter;
import ru.yandex.monlib.metrics.primitives.LazyGaugeInt64;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
public class CompositeMetricSupplierTest {

    @Test
    public void accept() {
        ShardMetrics shardJunk = new ShardMetrics("junk");
        shardJunk.requestStarted.add(10);
        shardJunk.requestCompleted.add(8);

        ShardMetrics shardSolomon = new ShardMetrics("solomon");
        shardSolomon.requestStarted.add(100);
        shardSolomon.requestCompleted.add(80);

        CompositeMetricSupplier provider = new CompositeMetricSupplier(
                Arrays.asList(shardJunk, shardSolomon),
                Labels.of("project", "alerting"));

        assertThat("(started, completed, inFlight) * 2", provider.estimateCount(), equalTo(6));

        final long now = System.currentTimeMillis();
        MetricsData metrics = getMetricsData(provider, now);
        assertThat(metrics.getCommonTsMillis(), equalTo(now));
        assertThat(metrics.getCommonLabels(), equalTo(Labels.of("project", "alerting")));
        checkShardMetrics(shardJunk, metrics);
        checkShardMetrics(shardSolomon, metrics);
    }

    @Test
    public void append() {
        ShardMetrics shardJunk = new ShardMetrics("junk");
        shardJunk.requestStarted.add(10);
        shardJunk.requestCompleted.add(8);

        ShardMetrics shardSolomon = new ShardMetrics("solomon");
        shardSolomon.requestStarted.add(100);
        shardSolomon.requestCompleted.add(80);

        CompositeMetricSupplier shardMetrics = new CompositeMetricSupplier(
                Arrays.asList(shardJunk, shardSolomon),
                Labels.of("subsystem", "alerting"));

        CompositeMetricSupplier root = new CompositeMetricSupplier(
                Arrays.asList(new MetricRegistry(), shardMetrics),
                Labels.of("project", "test"));

        assertThat("(started, completed, inFlight) * 2", root.estimateCount(), equalTo(6));

        final long now = System.currentTimeMillis();
        MetricsData metrics = getMetricsData(root, now);
        assertThat(metrics.getCommonTsMillis(), equalTo(now));
        assertThat(metrics.getCommonLabels(), equalTo(Labels.of("project", "test")));
        {
            Metric metric = metrics.getMetric(Labels.of(
                    "shardId", "junk",
                    "sensor", "inFlight",
                    "subsystem", "alerting"));
            assertThat(metric.type, equalTo(MetricType.IGAUGE));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, 2)));
        }
        {
            Metric metric = metrics.getMetric(Labels.of(
                    "shardId", "solomon",
                    "sensor", "inFlight",
                    "subsystem", "alerting"));
            assertThat(metric.type, equalTo(MetricType.IGAUGE));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, 20)));
        }
    }

    private void checkShardMetrics(ShardMetrics shard, MetricsData metrics) {
        {
            Metric metric = metrics.getMetric(Labels.of(
                    "shardId", shard.shardId,
                    "sensor", "requestStarted"));
            assertThat(metric.type, equalTo(MetricType.COUNTER));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, shard.requestStarted.get())));
        }
        {
            Metric metric = metrics.getMetric(Labels.of(
                    "shardId", shard.shardId,
                    "sensor", "requestCompleted"));
            assertThat(metric.type, equalTo(MetricType.COUNTER));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, shard.requestCompleted.get())));
        }
        {
            Metric metric = metrics.getMetric(Labels.of(
                    "shardId", shard.shardId,
                    "sensor", "inFlight"));
            assertThat(metric.type, equalTo(MetricType.IGAUGE));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, shard.inFlight.get())));
        }
    }

    private MetricsData getMetricsData(MetricSupplier provider, long now) {
        try (TestMetricConsumer consumer = new TestMetricConsumer()) {
            provider.supply(now, consumer);
            return consumer.getMetricsData();
        }
    }

    private class ShardMetrics implements MetricSupplier {
        private final String shardId;
        private final MetricRegistry registry;
        private final Counter requestStarted;
        private final Counter requestCompleted;
        private final LazyGaugeInt64 inFlight;

        public ShardMetrics(String shardId) {
            this.shardId = shardId;
            this.registry = new MetricRegistry(Labels.of("shardId", shardId));
            this.requestStarted = registry.counter("requestStarted");
            this.requestCompleted = registry.counter("requestCompleted");
            this.inFlight = registry.lazyGaugeInt64("inFlight", () -> requestStarted.get() - requestCompleted.get());
        }

        @Override
        public int estimateCount() {
            return registry.estimateCount();
        }

        @Override
        public void append(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
            registry.append(tsMillis, commonLabels, consumer);
        }
    }
}
