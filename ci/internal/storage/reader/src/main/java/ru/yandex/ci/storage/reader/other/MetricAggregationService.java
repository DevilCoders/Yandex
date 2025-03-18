package ru.yandex.ci.storage.reader.other;

import java.util.HashMap;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metric;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.MetricFunctionUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.utils.MapUtils;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;

@Slf4j
@RequiredArgsConstructor
public class MetricAggregationService {
    private final ReaderStatistics statistics;

    public Metrics merge(Metrics left, Metrics right) {
        var updatedMetrics = Stream.concat(left.getValues().entrySet().stream(), right.getValues().entrySet().stream())
                .collect(Collectors.toUnmodifiableMap(
                        Map.Entry::getKey, Map.Entry::getValue, MapUtils::max
                ));

        return new Metrics(updatedMetrics);
    }

    public Metrics aggregate(Metrics aggregate, Metrics old, Metrics updated) {
        var names = StreamEx.ofKeys(aggregate.getValues())
                .append(StreamEx.ofKeys(old.getValues()))
                .append(StreamEx.ofKeys(updated.getValues()))
                .toSet();

        var result = new HashMap<String, Metric>(aggregate.getValues().size());
        for (var name : names) {
            var aggregated = aggregate.getValues().get(name);
            var oldMetric = old.getValues().get(name);
            var updatedMetric = updated.getValues().get(name);

            if (updatedMetric == null) {
                result.put(name, aggregated);
                continue;
            }

            if (aggregated == null) {
                result.put(name, updatedMetric);
                continue;
            }

            result.put(name, aggregate(aggregated, oldMetric, updatedMetric));
        }
        return new Metrics(result);
    }

    public Metric aggregate(Metric aggregate, @Nullable Metric old, Metric delta) {
        if (aggregate.getFunction() != delta.getFunction()) {
            log.warn(
                    "Inconsistent aggregate and delta function {} and {}",
                    aggregate.getFunction(), delta.getFunction()
            );
            statistics.getMain().onMetricInconsistency();
            return aggregate;
        }

        if (aggregate.getSize() != delta.getSize()) {
            log.warn("Inconsistent metric and delta size {} and {}", aggregate.getSize(), delta.getSize());
            return aggregate;
        }

        if (old == null) {
            return aggregateWithoutDiff(aggregate, delta);
        }

        if (aggregate.getFunction() != old.getFunction()) {
            log.warn("Inconsistent aggregate and old function {} and {}", aggregate.getFunction(), delta.getFunction());
            return aggregate;
        }

        if (aggregate.getSize() != old.getSize()) {
            log.warn("Inconsistent metric and old size {} and {}", aggregate.getSize(), delta.getSize());
            return aggregate;
        }

        return new Metric(
                MetricFunctionUtils.aggregate(
                        aggregate.getFunction(), aggregate.getValue(), old.getValue(), delta.getValue()
                ),
                aggregate.getFunction(),
                aggregate.getSize()
        );
    }

    private Metric aggregateWithoutDiff(Metric aggregate, Metric delta) {
        return new Metric(
                MetricFunctionUtils.apply(aggregate.getFunction(), aggregate.getValue(), delta.getValue()),
                aggregate.getFunction(),
                aggregate.getSize()
        );
    }

}
