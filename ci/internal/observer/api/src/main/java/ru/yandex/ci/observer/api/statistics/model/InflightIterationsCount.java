package ru.yandex.ci.observer.api.statistics.model;

import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.LongSummaryStatistics;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.ToString;
import org.apache.commons.lang3.tuple.Pair;

import yandex.cloud.repository.db.IsolationLevel;
import ru.yandex.ci.observer.api.statistics.aggregated.CheckTypeAndCheckIterationType;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationsTable.CountByCheckTypeAdvisedPoolIterationType;
import ru.yandex.ci.observer.core.utils.ObserverStatisticConstants;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

import static java.util.stream.Collectors.groupingBy;
import static java.util.stream.Collectors.summarizingLong;

@ToString
public class InflightIterationsCount extends StatisticsItem {
    private static final String SENSOR = "inflight_iterations_count";

    public InflightIterationsCount(@Nonnull Map<String, String> labels, double value) {
        super(SENSOR, labels, value);
    }

    public InflightIterationsCount(
            @Nonnull CheckOuterClass.CheckType checkType,
            @Nonnull String advisedPool,
            @Nonnull CheckIteration.IterationType iterationType,
            double value
    ) {
        this(checkType, advisedPool, iterationType.toString(), value);
    }

    public InflightIterationsCount(
            @Nonnull CheckOuterClass.CheckType checkType,
            @Nonnull String advisedPool,
            @Nonnull String iterationType,
            double value
    ) {
        super(
                SENSOR,
                createSolomonLabels(checkType, advisedPool, iterationType),
                value
        );
    }

    public static List<InflightIterationsCount> compute(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes,
            @Nonnull CiObserverDb db
    ) {
        var inflightsCount = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getInflightAggregation(from, to, checkTypes));

        var result = inflightsCount.stream()
                .map(g -> new InflightIterationsCount(
                        g.getCheckType(), g.getAdvisedPool(), g.getIterationType(), g.getCount()
                )).collect(Collectors.toCollection(ArrayList::new));

        result.addAll(
                inflightsCount.stream().collect(
                        groupingBy(
                                it -> Pair.of(it.getCheckType(), it.getAdvisedPool()),
                                summarizingLong(CountByCheckTypeAdvisedPoolIterationType::getCount)
                        )
                ).entrySet().stream().map(entry -> new InflightIterationsCount(
                        entry.getKey().getLeft(),
                        entry.getKey().getRight(),
                        SensorLabels.ANY_CHECK_ITERATION_TYPE,
                        entry.getValue().getSum()
                )).toList()
        );

        var byCheckAndIterType = groupByCheckAndIterType(inflightsCount, checkTypes);
        result.addAll(
                byCheckAndIterType.entrySet().stream().map(entry -> new InflightIterationsCount(
                        entry.getKey().getCheckType(),
                        SensorLabels.ANY_POOL,
                        entry.getKey().getIterationType(),
                        entry.getValue().getSum()
                )).toList()
        );

        var byCheckType = groupByCheckType(inflightsCount, checkTypes);
        result.addAll(
                byCheckType.entrySet().stream().map(entry -> new InflightIterationsCount(
                        entry.getKey(),
                        SensorLabels.ANY_POOL,
                        SensorLabels.ANY_CHECK_ITERATION_TYPE,
                        entry.getValue().getSum()
                )).toList()
        );

        return result;
    }

    private static Map<CheckTypeAndCheckIterationType, LongSummaryStatistics> groupByCheckAndIterType(
            List<CountByCheckTypeAdvisedPoolIterationType> inflightsCount,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        var byCheckAndIterType = inflightsCount.stream().collect(
                groupingBy(
                        CheckTypeAndCheckIterationType::of,
                        HashMap::new,
                        summarizingLong(CountByCheckTypeAdvisedPoolIterationType::getCount)
                )
        );

        // add zeros to prevent NO DATA in solomon
        for (var checkType : checkTypes) {
            for (var iterType : ObserverStatisticConstants.ITERATION_TYPES_USED_IN_STATISTICS) {
                var key = CheckTypeAndCheckIterationType.of(checkType, iterType);
                byCheckAndIterType.computeIfAbsent(key, k -> new LongSummaryStatistics());
            }
        }

        return byCheckAndIterType;
    }

    private static Map<CheckOuterClass.CheckType, LongSummaryStatistics> groupByCheckType(
            List<CountByCheckTypeAdvisedPoolIterationType> inflightsCount,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        var byCheckType = inflightsCount.stream().collect(
                groupingBy(
                        CountByCheckTypeAdvisedPoolIterationType::getCheckType,
                        HashMap::new,
                        summarizingLong(CountByCheckTypeAdvisedPoolIterationType::getCount)
                )
        );
        // add zeros to prevent NO DATA in solomon
        for (var checkType : checkTypes) {
            byCheckType.computeIfAbsent(checkType, k -> new LongSummaryStatistics());
        }
        return byCheckType;
    }

    private static Map<String, String> createSolomonLabels(@Nonnull CheckOuterClass.CheckType checkType,
                                                           @Nonnull String advisedPool, @Nonnull String iterationType) {
        return Map.of(
                SensorLabels.CHECK_TYPE, checkType.toString(),
                SensorLabels.POOL, advisedPool,
                SensorLabels.CHECK_ITERATION_TYPE, iterationType
        );
    }
}
