package ru.yandex.monlib.metrics;

import java.util.Arrays;
import java.util.stream.Stream;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricsData.Metric;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.primitives.Counter;
import ru.yandex.monlib.metrics.primitives.GaugeDouble;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.Rate;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
public class ManualMetricAggregateTest {

    @Test
    public void combine() {
        Metrics junk = new Metrics(Labels.of("shardId", "junk"));
        junk.counter.add(1);
        junk.gaugeDouble.set(5d);
        junk.gaugeInt64.set(10);
        junk.histogram.record(22);
        junk.rate.add(100);

        Metrics solomon = new Metrics(Labels.of("shardId", "solomon"));
        solomon.counter.add(2);
        solomon.gaugeDouble.set(3);
        solomon.gaugeInt64.set(45);
        solomon.histogram.record(12);
        solomon.rate.add(333);

        Metrics kikimr = new Metrics(Labels.of("shardId", "kikimr"));
        kikimr.counter.add(6);
        kikimr.gaugeDouble.set(42);
        kikimr.gaugeInt64.set(8);
        kikimr.histogram.record(5, 20);
        kikimr.rate.add(10);

        Metrics total = Stream.of(junk, solomon, kikimr)
                .collect(
                        () -> new Metrics(Labels.of("shardId", "total")),
                        Metrics::append,
                        Metrics::combine
                );

        final long now = System.currentTimeMillis();
        MetricsData metrics = getMetricsData(now, junk, solomon, kikimr, total);
        assertThat(metrics.getCommonTsMillis(), equalTo(now));
        assertThat(metrics.getCommonLabels(), equalTo(Labels.empty()));
        assertThat(metrics.size(), equalTo(20));

        {
            Metric metric = metrics.getMetric(Labels.of("shardId", "total", "sensor", "myCounter"));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, 9)));
        }
        {
            Metric metric = metrics.getMetric(Labels.of("shardId", "total", "sensor", "myGaugeDouble"));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newDouble(0, 50d)));
        }
        {
            Metric metric = metrics.getMetric(Labels.of("shardId", "total", "sensor", "myGaugeInt64"));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, 63)));
        }
        {
            Metric metric = metrics.getMetric(Labels.of("shardId", "total", "sensor", "myHistogram"));
            HistogramSnapshot snapshot = new ExplicitHistogramSnapshot(
                new double[]{10, 20, 30, Histograms.INF_BOUND},
                new long[]{20, 1, 1, 0});
            assertThat(metric.timeSeries, equalTo(TimeSeries.newHistogram(0, snapshot)));
        }
        {
            Metric metric = metrics.getMetric(Labels.of("shardId", "total", "sensor", "myRate"));
            assertThat(metric.timeSeries, equalTo(TimeSeries.newLong(0, 443)));
        }
    }

    private MetricsData getMetricsData(long now, MetricSupplier... providers) {
        CompositeMetricSupplier provider = new CompositeMetricSupplier(Arrays.asList(providers));
        try (TestMetricConsumer consumer = new TestMetricConsumer()) {
            provider.supply(now, consumer);
            return consumer.getMetricsData();
        }
    }

    private static class Metrics implements MetricSupplier {
        private final MetricRegistry registry;
        private final Counter counter;
        private final GaugeDouble gaugeDouble;
        private final GaugeInt64 gaugeInt64;
        private final Histogram histogram;
        private final Rate rate;

        public Metrics(Labels commonLabels) {
            this.registry = new MetricRegistry(commonLabels);
            this.counter = registry.counter("myCounter");
            this.gaugeDouble = registry.gaugeDouble("myGaugeDouble");
            this.gaugeInt64 = registry.gaugeInt64("myGaugeInt64");
            this.histogram = registry.histogramCounter("myHistogram", Histograms.explicit(10, 20, 30));
            this.rate = registry.rate("myRate");
        }

        @Override
        public int estimateCount() {
            return registry.estimateCount();
        }

        public void append(Metrics metrics) {
            counter.combine(metrics.counter);
            gaugeDouble.combine(metrics.gaugeDouble);
            gaugeInt64.combine(metrics.gaugeInt64);
            histogram.combine(metrics.histogram);
            rate.combine(metrics.rate);
        }

        public Metrics combine(Metrics metrics) {
            append(metrics);
            return this;
        }

        @Override
        public void append(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
            registry.append(tsMillis, commonLabels, consumer);
        }
    }
}
