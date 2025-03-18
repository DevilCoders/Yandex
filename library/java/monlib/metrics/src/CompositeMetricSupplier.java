package ru.yandex.monlib.metrics;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import ru.yandex.monlib.metrics.labels.Labels;

/**
 * @author Vladimir Gordiychuk
 */
public class CompositeMetricSupplier implements MetricSupplier {
    private final List<MetricSupplier> suppliers;
    private final Labels commonLabels;

    public CompositeMetricSupplier(Collection<MetricSupplier> suppliers) {
        this(suppliers, Labels.empty());
    }

    public CompositeMetricSupplier(Collection<MetricSupplier> suppliers, Labels commonLabels) {
        this.suppliers = new ArrayList<>(suppliers);
        this.commonLabels = commonLabels;
    }

    @Override
    public int estimateCount() {
        return suppliers.stream()
                .mapToInt(MetricSupplier::estimateCount)
                .sum();
    }

    @Override
    public void supply(long tsMillis, MetricConsumer consumer) {
        consumer.onStreamBegin(estimateCount());
        consumer.onCommonTime(tsMillis);
        // on top level we put current registry common labels
        // outside from each metric
        if (!commonLabels.isEmpty()) {
            consumer.onLabelsBegin(commonLabels.size());
            commonLabels.forEach(consumer::onLabel);
            consumer.onLabelsEnd();
        }
        appendAll(0, Labels.empty(), consumer);
        consumer.onStreamEnd();
    }

    @Override
    public void append(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
        Labels mergedCommonLabels = commonLabels.addAll(this.commonLabels);
        appendAll(tsMillis, mergedCommonLabels, consumer);
    }

    private void appendAll(long tsMillis, Labels commonLabels, MetricConsumer consumer) {
        for (MetricSupplier provider : suppliers) {
            provider.append(tsMillis, commonLabels, consumer);
        }
    }
}
