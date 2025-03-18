package ru.yandex.monlib.metrics;

import java.time.Instant;
import java.util.HashMap;
import java.util.Map;

import org.junit.Assert;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.series.TimeSeries;


/**
 * @author Sergey Polovko
 */
public class MetricsData {

    private long commonTsMillis = 0;
    private Labels commonLabels = Labels.empty();
    private Map<Labels, Metric> metrics = new HashMap<>();

    public long getCommonTsMillis() {
        return commonTsMillis;
    }

    public void setCommonTsMillis(long commonTsMillis) {
        this.commonTsMillis = commonTsMillis;
    }

    public Labels getCommonLabels() {
        return commonLabels;
    }

    public void addCommonLabel(Label label) {
        commonLabels = commonLabels.add(label);
    }

    public int size() {
        return metrics.size();
    }

    public void addMetric(Labels labels, MetricType type, TimeSeries timeSeries) {
        Metric prev = metrics.put(labels, new Metric(type, timeSeries));
        if (prev != null) {
            Assert.fail("metric duplication: " + labels);
        }
    }

    public Metric getMetric(Labels labels) {
        Metric metric = metrics.get(labels);
        if (metric == null) {
            Assert.fail("metric " + labels + " not found");
        }
        return metric;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder()
            .append("MetricData{\n")
            .append("    commonTsMillis: ").append(Instant.ofEpochMilli(commonTsMillis)).append('\n')
            .append("    commonLabels: ").append(commonLabels).append('\n')
            .append("    metrics: [\n");

        for (Map.Entry<Labels, Metric> e : metrics.entrySet()) {
            sb.append("        ").append(e.getKey()).append(" => ").append(e.getValue()).append('\n');
        }

        return sb
            .append("    ]\n")
            .append('}')
            .toString();
    }

    /**
     * METRIC DATA
     */
    public static class Metric {
        public final MetricType type;
        public final TimeSeries timeSeries;

        Metric(MetricType type, TimeSeries timeSeries) {
            this.type = type;
            this.timeSeries = timeSeries;
        }

        @Override
        public String toString() {
            return type.name() + ": " + timeSeries;
        }
    }
}
