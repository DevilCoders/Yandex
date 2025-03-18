package ru.yandex.monlib.metrics.log4j2;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.junit.Before;
import org.junit.Test;

import ru.yandex.devtools.test.annotations.YaIgnore;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricType;
import ru.yandex.monlib.metrics.histogram.HistogramSnapshot;
import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.labels.LabelsBuilder;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.hasItem;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

/**
 * @author Vladimir Gordiychuk
 */
@YaIgnore // this test never pass via ya make because this tool replace all login by own implementation
public class InstrumentedAppenderTest {

    private Map<Labels, Long> previousPull = Collections.emptyMap();

    @Before
    public void setUp() {
        pullMetrics();
    }

    @Test
    public void hasEventsCountMetrics() {
        Logger logger = LogManager.getLogger(InstrumentedAppenderTest.class);
        Map<Labels, Long> metrics = pullMetrics();

        assertThat(metrics.keySet(), hasItem(eventCount(Level.ERROR)));
        assertThat(metrics.keySet(), hasItem(eventCount(Level.WARN)));
        assertThat(metrics.keySet(), hasItem(eventCount(Level.INFO)));
    }

    @Test
    public void error() {
        Logger logger = LogManager.getLogger(InstrumentedAppenderTest.class);
        logger.error("One");
        logger.error("Two");
        logger.error("Tree");

        long count = pullMetrics().get(eventCount(Level.ERROR));
        assertEquals(3, count);
    }

    @Test
    public void warn() {
        Logger logger = LogManager.getLogger(InstrumentedAppenderTest.class);
        logger.warn("test");

        long count = pullMetrics().get(eventCount(Level.WARN));
        assertEquals(1, count);
    }

    @Test
    public void trace() {
        Logger logger = LogManager.getLogger(InstrumentedAppenderTest.class);
        logger.trace("trace message");

        long count = pullMetrics().get(eventCount(Level.TRACE));
        assertEquals(0, count);
    }

    @Test
    public void debug() {
        Logger logger = LogManager.getLogger(InstrumentedAppenderTest.class);
        logger.debug("debug message 1");
        logger.debug("debug message 2");
        logger.debug("debug message 3");
        logger.debug("debug message 4");

        long count = pullMetrics().get(eventCount(Level.DEBUG));
        assertEquals(4, count);
    }

    @Test
    public void rootLogger() {
        Logger logger = LogManager.getLogger("mySuperLogger");
        logger.debug("Debug");
        logger.info("Info");
        logger.trace("Trace");
        logger.error("Error");
        logger.fatal("Fatal");

        Map<Labels, Long> metrics = pullMetrics();
        assertThat(metrics.get(eventCount(Level.DEBUG)), equalTo(0L));
        assertThat(metrics.get(eventCount(Level.INFO)), equalTo(0L));
        assertThat(metrics.get(eventCount(Level.TRACE)), equalTo(0L));
        assertThat(metrics.get(eventCount(Level.ERROR)), equalTo(1L));
        assertThat(metrics.get(eventCount(Level.FATAL)), equalTo(1L));
    }

    private Labels eventCount(Level level) {
        return Labels.of(
                "sensor", "log4j.events",
                "level", level.name()
        );
    }

    private Map<Labels, Long> pullMetrics() {
        Consumer consumer = new Consumer();
        MetricRegistry.root().supply(0, consumer);
        Map<Labels, Long> values = consumer.getLabelToValue();
        Map<Labels, Long> result = diff(previousPull, values);
        previousPull = values;
        return result;
    }

    private Map<Labels, Long> diff(Map<Labels, Long> prev, Map<Labels, Long> now) {
        Map<Labels, Long> result = new HashMap<>(now.size());
        for (Map.Entry<Labels, Long> entry : now.entrySet()) {
            long value = prev.getOrDefault(entry.getKey(), 0L);
            result.put(entry.getKey(), entry.getValue() - value);
        }
        return result;
    }

    private class Consumer implements MetricConsumer {
        private final Map<Labels, Long> labelToValue = new HashMap<>();

        private boolean skip = false;
        private long value;
        private Labels labels;
        private LabelsBuilder labelsBuilder;

        public Map<Labels, Long> getLabelToValue() {
            return labelToValue;
        }

        @Override
        public void onStreamBegin(int countHint) {
        }

        @Override
        public void onStreamEnd() {
        }

        @Override
        public void onCommonTime(long tsMillis) {
        }

        @Override
        public void onMetricBegin(MetricType type) {
            skip = type != MetricType.RATE;
            value = 0;
            labels = Labels.empty();
        }

        @Override
        public void onMetricEnd() {
            if (skip) {
                return;
            }

            labelToValue.put(labels, value);
        }

        @Override
        public void onLabelsBegin(int countHint) {
            labelsBuilder = new LabelsBuilder(countHint);
        }

        @Override
        public void onLabelsEnd() {
            labels = labelsBuilder.build();
            labelsBuilder.clear();
        }

        @Override
        public void onLabel(Label label) {
            labelsBuilder.add(label);
        }

        @Override
        public void onDouble(long tsMillis, double value) {
            this.value = (long) value;
        }

        @Override
        public void onLong(long tsMillis, long value) {
            this.value = value;
        }

        @Override
        public void onHistogram(long tsMillis, HistogramSnapshot snapshot) {
        }
    }
}
