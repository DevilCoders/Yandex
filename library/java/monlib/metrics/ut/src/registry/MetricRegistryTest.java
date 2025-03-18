package ru.yandex.monlib.metrics.registry;

import java.lang.management.ManagementFactory;
import java.util.function.LongSupplier;

import org.junit.Test;

import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.MetricsData;
import ru.yandex.monlib.metrics.MetricsData.Metric;
import ru.yandex.monlib.metrics.TestMetricConsumer;
import ru.yandex.monlib.metrics.histogram.ExplicitHistogramSnapshot;
import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.meter.Meter;
import ru.yandex.monlib.metrics.primitives.Counter;
import ru.yandex.monlib.metrics.primitives.GaugeDouble;
import ru.yandex.monlib.metrics.primitives.GaugeInt64;
import ru.yandex.monlib.metrics.primitives.Histogram;
import ru.yandex.monlib.metrics.primitives.LazyCounter;
import ru.yandex.monlib.metrics.series.TimeSeries;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;


/**
 * @author Sergey Polovko
 */
public class MetricRegistryTest {

    @Test
    public void counter() throws Exception {
        MetricRegistry registry = new MetricRegistry();

        {
            Counter c1 = registry.counter("a");
            assertNotNull(c1);
            Counter c2 = registry.counter("a");
            assertNotNull(c2);
            assertSame(c1, c2);
        }
        {
            Counter c1 = registry.counter("a");
            assertNotNull(c1);
            Counter c2 = registry.counter("b");
            assertNotNull(c2);
            assertNotSame(c1, c2);
        }
        {
            Counter c1 = registry.counter("b", Labels.of("c", "d"));
            assertNotNull(c1);
            Counter c2 = registry.counter("b", Labels.of("c", "d"));
            assertNotNull(c2);
            assertSame(c1, c2);
        }
        {
            Counter c1 = registry.counter("b", Labels.of("c", "d"));
            assertNotNull(c1);
            Counter c2 = registry.counter("b", Labels.of("c", "e"));
            assertNotNull(c2);
            assertNotSame(c1, c2);
        }
    }

    @Test
    public void gaugeDouble() throws Exception {
        MetricRegistry registry = new MetricRegistry();

        {
            GaugeDouble g1 = registry.gaugeDouble("a");
            assertNotNull(g1);
            GaugeDouble g2 = registry.gaugeDouble("a");
            assertNotNull(g2);
            assertSame(g1, g2);
        }
        {
            GaugeDouble g1 = registry.gaugeDouble("a");
            assertNotNull(g1);
            GaugeDouble g2 = registry.gaugeDouble("b");
            assertNotNull(g2);
            assertNotSame(g1, g2);
        }
        {
            GaugeDouble g1 = registry.gaugeDouble("b", Labels.of("c", "d"));
            assertNotNull(g1);
            GaugeDouble g2 = registry.gaugeDouble("b", Labels.of("c", "d"));
            assertNotNull(g2);
            assertSame(g1, g2);
        }
        {
            GaugeDouble g1 = registry.gaugeDouble("b", Labels.of("c", "d"));
            assertNotNull(g1);
            GaugeDouble g2 = registry.gaugeDouble("b", Labels.of("c", "e"));
            assertNotNull(g2);
            assertNotSame(g1, g2);
        }
    }

    @Test
    public void gaugeInt64() throws Exception {
        MetricRegistry registry = new MetricRegistry();

        {
            GaugeInt64 g1 = registry.gaugeInt64("a");
            assertNotNull(g1);
            GaugeInt64 g2 = registry.gaugeInt64("a");
            assertNotNull(g2);
            assertSame(g1, g2);
        }
        {
            GaugeInt64 g1 = registry.gaugeInt64("a");
            assertNotNull(g1);
            GaugeInt64 g2 = registry.gaugeInt64("b");
            assertNotNull(g2);
            assertNotSame(g1, g2);
        }
        {
            GaugeInt64 g1 = registry.gaugeInt64("b", Labels.of("c", "d"));
            assertNotNull(g1);
            GaugeInt64 g2 = registry.gaugeInt64("b", Labels.of("c", "d"));
            assertNotNull(g2);
            assertSame(g1, g2);
        }
        {
            GaugeInt64 g1 = registry.gaugeInt64("b", Labels.of("c", "d"));
            assertNotNull(g1);
            GaugeInt64 g2 = registry.gaugeInt64("b", Labels.of("c", "e"));
            assertNotNull(g2);
            assertNotSame(g1, g2);
        }
    }

    @Test
    public void lazyGaugeDouble() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry registry = new MetricRegistry();
        registry.lazyGaugeDouble("pi", () -> Math.PI);
        registry.lazyGaugeDouble("sin(pi/2)", () -> Math.sin(Math.PI / 2));

        MetricsData metrics = getMetricsData(registry, now);
        assertEquals(2, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "pi"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, Math.PI), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "sin(pi/2)"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 1.0), metric.timeSeries);
        }
    }

    @Test
    public void lazyGaugeInt64() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry registry = new MetricRegistry();
        registry.lazyGaugeInt64("min", () -> Long.MIN_VALUE);
        registry.lazyGaugeInt64("max", () -> Long.MAX_VALUE);

        MetricsData metrics = getMetricsData(registry, now);
        assertEquals(2, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "min"));
            assertEquals(MetricType.IGAUGE, metric.type);
            assertEquals(TimeSeries.newLong(0, Long.MIN_VALUE), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "max"));
            assertEquals(MetricType.IGAUGE, metric.type);
            assertEquals(TimeSeries.newLong(0, Long.MAX_VALUE), metric.timeSeries);
        }
    }

    @Test
    public void accept() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry registry = new MetricRegistry(Labels.of("service", "stockpile"));

        Counter counter = registry.counter("myCounter", Labels.of("cluster", "production"));
        counter.inc();

        GaugeDouble gauge = registry.gaugeDouble("myGauge", Labels.of("cluster", "testing"));
        gauge.set(3.14159);

        {
            MetricRegistry httpServer = registry.subRegistry("subsystem", "httpServer");
            Counter requestsCount = httpServer.counter("requestsCount");
            requestsCount.add(1000);
        }

        MetricsData metrics = getMetricsData(registry, now);

        assertEquals(Labels.of("service", "stockpile"), metrics.getCommonLabels());
        assertEquals(now, metrics.getCommonTsMillis());
        assertEquals(3, metrics.size());

        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "myCounter", "cluster", "production"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 1), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "myGauge", "cluster", "testing"));
            assertEquals(MetricType.DGAUGE, metric.type);
            assertEquals(TimeSeries.newDouble(0, 3.14159), metric.timeSeries);
        }
        {
            Metric metric = metrics.getMetric(Labels.of("sensor", "requestsCount", "subsystem", "httpServer"));
            assertEquals(MetricType.COUNTER, metric.type);
            assertEquals(TimeSeries.newLong(0, 1000), metric.timeSeries);
        }
    }

    @Test
    public void acceptSubRegistryHierarchy() {
        final long now = System.currentTimeMillis();
        MetricRegistry root = new MetricRegistry();
        MetricRegistry levelOne = root.subRegistry("endpoint", "/ok");
        MetricRegistry levelTwo = levelOne.subRegistry("method", "GET");
        MetricRegistry levelTree = levelTwo.subRegistry("user", "solomon");
        GaugeInt64 gauge = levelTree.gaugeInt64("elapsedTime", Labels.of("unit", "ms"));
        gauge.set(42);

        MetricsData metrics = getMetricsData(root, now);
        Metric metric = metrics.getMetric(Labels.builder()
                .add("endpoint", "/ok")
                .add("method", "GET")
                .add("user", "solomon")
                .add("sensor", "elapsedTime")
                .add("unit", "ms")
                .build());
        assertEquals(MetricType.IGAUGE, metric.type);
        assertEquals(TimeSeries.newLong(0, 42), metric.timeSeries);
    }

    @Test
    public void acceptAsAProviders() {
        MetricRegistry shardOne = new MetricRegistry(Labels.of("shardId", "junk"));
        shardOne.counter("countOfUnits").add(42);

        MetricRegistry shardTwo = new MetricRegistry(Labels.of("shardId", "solomon"));
        shardTwo.counter("countOfUnits").add(55);

        try (TestMetricConsumer consumer = new TestMetricConsumer()) {
            consumer.onStreamBegin(shardOne.estimateCount() + shardTwo.estimateCount());
            consumer.onCommonTime(System.currentTimeMillis());
            shardOne.append(0, Labels.empty(), consumer);
            shardTwo.append(0, Labels.empty(), consumer);
            consumer.onStreamEnd();

            MetricsData metrics = consumer.getMetricsData();
            {
                Metric metric = metrics.getMetric(Labels.of("shardId", "junk", "sensor", "countOfUnits"));
                assertEquals(MetricType.COUNTER, metric.type);
                assertEquals(TimeSeries.newLong(0, 42), metric.timeSeries);
            }
            {
                Metric metric = metrics.getMetric(Labels.of("shardId", "solomon", "sensor", "countOfUnits"));
                assertEquals(MetricType.COUNTER, metric.type);
                assertEquals(TimeSeries.newLong(0, 55), metric.timeSeries);
            }
        }
    }

    @Test
    public void acceptHistogram() {
        MetricRegistry root = new MetricRegistry();
        MetricRegistry registry = root.subRegistry(Labels.of("subsystem", "test"));

        Histogram histogram = registry.histogramCounter("myHistogramCounter", Histograms.explicit(10, 20, 30));
        histogram.record(1);
        histogram.record(2);
        histogram.record(15);

        MetricsData metrics = getMetricsData(root, System.currentTimeMillis());

        Metric metric = metrics.getMetric(Labels.of("subsystem", "test", "sensor", "myHistogramCounter"));
        assertEquals(MetricType.HIST, metric.type);
        TimeSeries expectTs = TimeSeries.newHistogram(0,
                new ExplicitHistogramSnapshot(
                        new double[]{10, 20, 30, Histograms.INF_BOUND},
                        new long[]{2,  1,  0,  0}));
        assertEquals(expectTs, metric.timeSeries);
    }

    @Test
    public void acceptOnDifferentRegistryLevels() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry first = new MetricRegistry(Labels.of("first", "level"));
        first.counter("counter1");
        first.gaugeDouble("gauge1");

        MetricRegistry second = first.subRegistry("second", "level");
        second.counter("counter2");
        second.gaugeDouble("gauge2");

        MetricRegistry third = second.subRegistry("third", "level");
        third.counter("counter3");
        third.gaugeDouble("gauge3");

        MetricsData firstData = getMetricsData(first, now);
        assertEquals(now, firstData.getCommonTsMillis());
        assertEquals(Labels.of("first", "level"), firstData.getCommonLabels());
        assertEquals(6, firstData.size());
        assertEquals(MetricType.COUNTER, firstData.getMetric(Labels.of("sensor", "counter1")).type);
        assertEquals(MetricType.DGAUGE,   firstData.getMetric(Labels.of("sensor", "gauge1")).type);
        assertEquals(MetricType.COUNTER, firstData.getMetric(Labels.of("second", "level", "sensor", "counter2")).type);
        assertEquals(MetricType.DGAUGE,   firstData.getMetric(Labels.of("second", "level", "sensor", "gauge2")).type);
        assertEquals(MetricType.COUNTER, firstData.getMetric(Labels.of("second", "level", "third", "level", "sensor", "counter3")).type);
        assertEquals(MetricType.DGAUGE,   firstData.getMetric(Labels.of("second", "level", "third", "level", "sensor", "gauge3")).type);

        MetricsData secondData = getMetricsData(second, now);
        assertEquals(now, secondData.getCommonTsMillis());
        assertEquals(Labels.of("second", "level"), secondData.getCommonLabels());
        assertEquals(4, secondData.size());
        assertEquals(MetricType.COUNTER, secondData.getMetric(Labels.of("sensor", "counter2")).type);
        assertEquals(MetricType.DGAUGE,   secondData.getMetric(Labels.of("sensor", "gauge2")).type);
        assertEquals(MetricType.COUNTER, secondData.getMetric(Labels.of("third", "level", "sensor", "counter3")).type);
        assertEquals(MetricType.DGAUGE,   secondData.getMetric(Labels.of("third", "level", "sensor", "gauge3")).type);

        MetricsData thirdData = getMetricsData(third, now);
        assertEquals(now, thirdData.getCommonTsMillis());
        assertEquals(Labels.of("third", "level"), thirdData.getCommonLabels());
        assertEquals(2, thirdData.size());
        assertEquals(MetricType.COUNTER, thirdData.getMetric(Labels.of("sensor", "counter3")).type);
        assertEquals(MetricType.DGAUGE,   thirdData.getMetric(Labels.of("sensor", "gauge3")).type);
    }

    @Test
    public void overriding() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry first = new MetricRegistry(Labels.of("level", "first"));
        first.counter("counter1");
        first.gaugeDouble("gauge1");

        MetricRegistry second = first.subRegistry("level", "second");
        second.counter("counter2");
        second.gaugeDouble("gauge2");

        MetricRegistry third = second.subRegistry("level", "third");
        third.counter("counter3");
        third.gaugeDouble("gauge3");
        third.counter("counter4", Labels.of("level", "fake"));
        third.gaugeDouble("gauge4", Labels.of("level", "fake"));

        MetricsData data = getMetricsData(first, now);
        assertEquals(now, data.getCommonTsMillis());
        assertEquals(Labels.of("level", "first"), data.getCommonLabels());
        assertEquals(8, data.size());
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("sensor", "counter1")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("sensor", "gauge1")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("level", "second", "sensor", "counter2")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("level", "second", "sensor", "gauge2")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("level", "third", "sensor", "counter3")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("level", "third", "sensor", "gauge3")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("level", "fake", "sensor", "counter4")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("level", "fake", "sensor", "gauge4")).type);
    }

    @Test
    public void removeMetric() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry first = new MetricRegistry(Labels.of("first", "level"));
        first.counter("counter");
        first.gaugeDouble("gauge");

        MetricRegistry second = first.subRegistry("second", "level");
        second.counter("counter");
        second.gaugeDouble("gauge");

        MetricRegistry third = second.subRegistry("third", "level");
        third.counter("counter");
        third.gaugeDouble("gauge");

        MetricsData data = getMetricsData(first, now);
        assertEquals(6, data.size());

        assertNotNull(first.removeMetric("counter"));
        assertNull(first.removeMetric("counter"));

        data = getMetricsData(first, now);
        assertEquals(5, data.size());
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("sensor", "gauge")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("second", "level", "sensor", "counter")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "sensor", "gauge")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("second", "level", "third", "level", "sensor", "counter")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "third", "level", "sensor", "gauge")).type);

        assertNotNull(second.removeMetric("counter"));
        assertNull(second.removeMetric("counter"));

        data = getMetricsData(first, now);
        assertEquals(4, data.size());
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("sensor", "gauge")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "sensor", "gauge")).type);
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("second", "level", "third", "level", "sensor", "counter")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "third", "level", "sensor", "gauge")).type);

        assertNotNull(third.removeMetric("counter"));
        assertNull(third.removeMetric("counter"));

        data = getMetricsData(first, now);
        assertEquals(3, data.size());
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("sensor", "gauge")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "sensor", "gauge")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("second", "level", "third", "level", "sensor", "gauge")).type);
    }

    @Test
    public void removeSubRegistry() throws Exception {
        final long now = System.currentTimeMillis();

        MetricRegistry first = new MetricRegistry(Labels.of("first", "level"));
        first.counter("counter");
        first.gaugeDouble("gauge");

        MetricRegistry second = first.subRegistry("second", "level");
        second.counter("counter");
        second.gaugeDouble("gauge");

        MetricRegistry third = second.subRegistry("third", "level");
        third.counter("counter");
        third.gaugeDouble("gauge");

        MetricsData data = getMetricsData(first, now);
        assertEquals(6, data.size());

        assertNotNull(first.removeSubRegistry("second", "level"));
        assertNull(first.removeSubRegistry("second", "level"));

        data = getMetricsData(first, now);
        assertEquals(2, data.size());
        assertEquals(MetricType.COUNTER, data.getMetric(Labels.of("sensor", "counter")).type);
        assertEquals(MetricType.DGAUGE,   data.getMetric(Labels.of("sensor", "gauge")).type);
    }

    @Test
    public void lazyCounter() {
        LongSupplier supplier = () -> ManagementFactory.getRuntimeMXBean().getUptime();
        MetricRegistry registry = new MetricRegistry();
        LazyCounter counterOne = registry.lazyCounter("system.upTime", supplier);
        LazyCounter counterTwo = registry.lazyCounter("system.upTime", supplier);

        assertSame(counterOne, counterTwo);
        assertNotEquals(0L, supplier.getAsLong());
    }

    @Test
    public void meter() {
        MetricRegistry registry = new MetricRegistry();

        {
            Meter m1 = registry.oneMinuteMeter("a");
            assertNotNull(m1);
            Meter m2 = registry.oneMinuteMeter("a");
            assertNotNull(m2);
            assertSame(m1, m2);
        }
        {
            Meter m1 = registry.oneMinuteMeter("a");
            assertNotNull(m1);
            Meter m2 = registry.oneMinuteMeter("b");
            assertNotNull(m2);
            assertNotSame(m1, m2);
        }
        {
            Meter m1 = registry.oneMinuteMeter("b", Labels.of("c", "d"));
            assertNotNull(m1);
            Meter m2 = registry.oneMinuteMeter("b", Labels.of("c", "d"));
            assertNotNull(m2);
            assertSame(m1, m2);
        }
        {
            Meter m1 = registry.oneMinuteMeter("b", Labels.of("c", "d"));
            assertNotNull(m1);
            Meter m2 = registry.oneMinuteMeter("b", Labels.of("c", "e"));
            assertNotNull(m2);
            assertNotSame(m1, m2);
        }
    }

    private MetricsData getMetricsData(MetricRegistry registry, long now) {
        try (TestMetricConsumer consumer = new TestMetricConsumer()) {
            registry.accept(now, consumer);
            return consumer.getMetricsData();
        }
    }
}
