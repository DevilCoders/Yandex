package ru.yandex.monlib.metrics.registry;

import java.util.Objects;

import javax.annotation.ParametersAreNonnullByDefault;
import javax.annotation.concurrent.Immutable;

import ru.yandex.monlib.metrics.labels.Label;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Sergey Polovko
 */
@Immutable
@ParametersAreNonnullByDefault
public final class MetricId {

    private final Label name;
    private final Labels labels;

    public MetricId(String name, Labels labels) {
        // temporary hack to avoid multiple labels allocation in registry
        this.name = Labels.allocator.alloc("sensor", name);
        this.labels = Objects.requireNonNull(labels, "labels");
    }

    public String getName() {
        return name.getValue();
    }

    Label getNameAsLabel() {
        return name;
    }

    public Labels getLabels() {
        return labels;
    }

    public int getLabelsCount() {
        return labels.size();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        MetricId metricId = (MetricId) o;
        if (!name.equals(metricId.name)) return false;
        return labels.equals(metricId.labels);
    }

    @Override
    public int hashCode() {
        int result = name.hashCode();
        result = 31 * result + labels.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return getName() + labels;
    }
}
