package ru.yandex.ci.storage.core.db.model.check_iteration.metrics;

import java.util.Map;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@AllArgsConstructor
public class Metrics {
    public static final Metrics EMPTY = new Metrics(Map.of());

    @Nonnull
    Map<String, Metric> values;

    public static Metrics withSingleMetric(String name, Metric metric) {
        return new Metrics(Map.of(name, metric));
    }

    public double getValueOrElse(String key, double defaultValue) {
        var metric = values.get(key);
        if (metric == null) {
            return defaultValue;
        }
        return metric.getValue();
    }

    public int getValueOrElse(String key, int defaultValue) {
        var metric = values.get(key);
        if (metric == null) {
            return defaultValue;
        }
        return (int) metric.getValue();
    }

    public double getValueOrZero(String key) {
        var metric = values.get(key);
        if (metric == null) {
            return 0;
        }
        return metric.getValue();
    }

    public static final class Name {

        public static final String MACHINE_HOURS = "machine_hours";
        public static final String NUMBER_OF_NODES = "number_of_nodes";
        public static final String CACHE_HIT = "cache_hit";

        private Name() {
        }
    }
}
