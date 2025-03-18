package ru.yandex.ci.storage.core.db.model.check_iteration.metrics;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@AllArgsConstructor
public class Metric implements Comparable<Metric> {
    double value;

    Common.MetricAggregateFunction function;
    Common.MetricSize size;

    @Override
    public int compareTo(Metric other) {
        return Double.compare(value, other.value);
    }
}
