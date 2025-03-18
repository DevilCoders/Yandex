package ru.yandex.monlib.metrics.registry;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.monlib.metrics.Metric;
import ru.yandex.monlib.metrics.MetricConsumer;
import ru.yandex.monlib.metrics.MetricSupplier;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Sergey Polovko
 */
public class MetricRegistry implements
    MetricSupplier,
    CounterFactory,
    GaugeFactory,
    RateFactory,
    HistogramFactory,
    MeterFactory
{

    private final ConcurrentHashMap<Labels, MetricRegistry> subRegistries;
    private final ConcurrentHashMap<MetricId, Metric> metrics;
    private final Labels commonLabels;

    public MetricRegistry() {
        this(Labels.empty());
    }

    public MetricRegistry(Labels commonLabels) {
        this.commonLabels = commonLabels;
        this.subRegistries = new ConcurrentHashMap<>();
        this.metrics = new ConcurrentHashMap<>();
    }

    public static MetricRegistry root() {
        return Holder.ROOT;
    }

    public MetricRegistry subRegistry(String key, String value) {
        return subRegistry(Labels.of(key, value));
    }

    public MetricRegistry subRegistry(Labels labels) {
        MetricRegistry subRegistry = subRegistries.get(labels);
        if (subRegistry != null) {
            return subRegistry;
        }
        return subRegistries.computeIfAbsent(labels, MetricRegistry::new);
    }

    public Labels getCommonLabels() {
        return commonLabels;
    }

    /**
     * @deprecated use {@link MetricRegistry#supply(long, MetricConsumer)} instead
     */
    @Deprecated
    public void accept(long tsMillis, MetricConsumer c) {
        supply(tsMillis, c);
    }

    @Override
    public void supply(long tsMillis, MetricConsumer c) {
        c.onStreamBegin(-1);
        c.onCommonTime(tsMillis);
        // on top level we put current registry common labels
        // outside from each metric
        if (!commonLabels.isEmpty()) {
            c.onLabelsBegin(commonLabels.size());
            commonLabels.forEach(c::onLabel);
            c.onLabelsEnd();
        }
        appendImpl(0, Labels.empty(), Labels.empty(), c);
        c.onStreamEnd();
    }

    private void appendImpl(long tsMillis, Labels parentLabels, Labels commonLabels, MetricConsumer c) {
        final int additionalLabelsCount = parentLabels.size() + commonLabels.size();
        for (Map.Entry<MetricId, Metric> e : metrics.entrySet()) {
            final MetricId metricId = e.getKey();
            final Metric metric = e.getValue();

            c.onMetricBegin(metric.type());
            {
                // used this order:
                //      1) parent labels
                //      2) common labels
                //      3) metric labels
                //      4) name
                // to be able to override more common part with more specific part

                c.onLabelsBegin(additionalLabelsCount + metricId.getLabelsCount() + 1);
                if (additionalLabelsCount > 0) {
                    parentLabels.forEach(c::onLabel);
                    commonLabels.forEach(c::onLabel);
                }
                metricId.getLabels().forEach(c::onLabel);
                c.onLabel(metricId.getNameAsLabel());
                c.onLabelsEnd();
            }
            metric.accept(tsMillis, c);
            c.onMetricEnd();
        }

        if (subRegistries.isEmpty()) {
            return;
        }

        Labels mergedParentLabels = parentLabels.addAll(commonLabels);
        for (MetricRegistry r : subRegistries.values()) {
            r.append(tsMillis, mergedParentLabels, c);
        }
    }

    @Override
    public int estimateCount() {
        int count = metrics.size();
        for (MetricRegistry registry : subRegistries.values()) {
            count += registry.estimateCount();
        }

        return count;
    }

    @Override
    public void append(long tsMillis, Labels parentLabels, MetricConsumer c) {
        appendImpl(tsMillis, parentLabels, commonLabels, c);
    }

    @Nullable
    public Metric removeMetric(String name) {
        return removeMetric(name, Labels.empty());
    }

    @Nullable
    public Metric removeMetric(String name, Labels labels) {
        return removeMetric(new MetricId(name, labels));
    }

    @Nullable
    public Metric removeMetric(MetricId metricId) {
        return metrics.remove(metricId);
    }

    @Nullable
    public MetricRegistry removeSubRegistry(String key, String value) {
        return removeSubRegistry(Labels.of(key, value));
    }

    @Nullable
    public MetricRegistry removeSubRegistry(Labels labels) {
        return subRegistries.remove(labels);
    }

    @Nullable
    @Override
    public Metric getMetric(MetricId id) {
        return metrics.get(id);
    }

    @Nonnull
    @Override
    public Metric putMetricIfAbsent(MetricId id, Metric metric) {
        Metric prevMetric = metrics.putIfAbsent(id, metric);
        if (prevMetric == null) {
            return metric;
        }
        return prevMetric;
    }

    /**
     * ROOT HOLDER
     */
    private static final class Holder {
        static final MetricRegistry ROOT = new MetricRegistry();
    }
}
