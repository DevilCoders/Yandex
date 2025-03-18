package ru.yandex.ci.observer.api.statistics.aggregated;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.IsolationLevel;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.observer.api.statistics.model.AggregatedStageDuration;
import ru.yandex.ci.observer.api.statistics.model.AggregatedWindowedStatistics;
import ru.yandex.ci.observer.api.statistics.model.InflightIterationsCount;
import ru.yandex.ci.observer.api.statistics.model.SensorLabels;
import ru.yandex.ci.observer.api.statistics.model.StageType;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationsTable.WindowedStatsByPoolCount;
import ru.yandex.ci.observer.core.db.model.sla_statistics.IterationCompleteGroup;
import ru.yandex.ci.observer.core.db.model.sla_statistics.IterationTypeGroup;
import ru.yandex.ci.observer.core.db.model.sla_statistics.SlaStatisticsEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

@Slf4j
public class AggregatedStatisticsService {
    @VisibleForTesting
    static final String TOTAL_STAGE = "total";

    private final Clock clock;
    private final CiObserverDb db;
    private final TimingPercentiles percentiles;
    private final List<Integer> percentileIndexes;
    private final List<String> stagesToAggregate;
    private final List<Duration> aggregateWindows;
    private final Duration lastSaveDelay;

    public AggregatedStatisticsService(
            Clock clock,
            CiObserverDb db,
            List<Integer> percentileIndexes,
            List<String> stagesToAggregate,
            List<Duration> aggregateWindows,
            Duration lastSaveDelay
    ) {
        this.clock = clock;
        this.db = db;
        this.percentiles = new TimingPercentiles(percentileIndexes);
        this.percentileIndexes = percentileIndexes;
        this.stagesToAggregate = stagesToAggregate;
        this.aggregateWindows = aggregateWindows;
        this.lastSaveDelay = lastSaveDelay;
    }

    public List<AggregatedStageDuration> getStagesPercentilesOfActiveIterations(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull CheckOuterClass.CheckType checkType,
            @Nonnull StageType stageType
    ) {
        return getStagesPercentilesOfActiveIterations(from, to, List.of(checkType), checkType.toString(), stageType,
                true);
    }

    /**
     * Compute duration seconds percentiles by stages of active iterations
     *
     * @param checkTypeLabel label used to identify set of collected iterations with check types
     * @param stageType      type of stages (@see {@link StageType})
     */
    public List<AggregatedStageDuration> getStagesPercentilesOfActiveIterations(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes,
            @Nonnull String checkTypeLabel,
            @Nonnull StageType stageType,
            boolean includeStressTestChecks
    ) {
        var aggregatedStages = new ArrayList<AggregatedStageDuration>();
        var iterations = getActiveTrunkPrecommitIterations(from, to, checkTypes, includeStressTestChecks);

        aggregatedStages.addAll(computeStagesDurationsPercentiles(
                SensorLabels.ANY_POOL, iterations, IterationType.ANY, stageType, checkTypeLabel
        ));
        aggregatedStages.addAll(computeStagesDurationsPercentiles(
                SensorLabels.ANY_POOL, iterations, IterationType.MAIN, stageType, checkTypeLabel
        ));
        aggregatedStages.addAll(computeStagesDurationsPercentiles(
                SensorLabels.ANY_POOL, iterations, IterationType.RECHECK, stageType, checkTypeLabel
        ));

        var iterationsByPools = iterations.stream()
                .collect(Collectors.groupingBy(CheckIterationEntity::getAdvisedPool));

        for (var iterationsByPool : iterationsByPools.entrySet()) {
            aggregatedStages.addAll(computeStagesDurationsPercentiles(
                    iterationsByPool.getKey(), iterationsByPool.getValue(),
                    IterationType.ANY, stageType, checkTypeLabel
            ));
            aggregatedStages.addAll(computeStagesDurationsPercentiles(
                    iterationsByPool.getKey(), iterationsByPool.getValue(),
                    IterationType.MAIN, stageType, checkTypeLabel
            ));
            aggregatedStages.addAll(computeStagesDurationsPercentiles(
                    iterationsByPool.getKey(), iterationsByPool.getValue(),
                    IterationType.RECHECK, stageType, checkTypeLabel
            ));
        }

        return aggregatedStages;
    }

    public List<InflightIterationsCount> getInflightIterationsCount(@Nonnull Instant from, @Nonnull Instant to,
                                                                    @Nonnull CheckOuterClass.CheckType checkTypes) {
        return getInflightIterationsCount(from, to, List.of(checkTypes));
    }

    public List<InflightIterationsCount> getInflightIterationsCount(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        return InflightIterationsCount.compute(from, to, checkTypes, db);
    }

    public List<AggregatedWindowedStatistics> getWindowedPassedFailedIterationsCount(
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        var maxWindow = aggregateWindows.stream().max(Duration::compareTo).orElse(Duration.ZERO);
        var fromWithMaxWindow = to.minus(maxWindow.toSeconds(), ChronoUnit.SECONDS);

        var finishedCountByStatusAndPools = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getFinishedStatusCount(fromWithMaxWindow, to, aggregateWindows, checkTypes));

        var result = finishedCountByStatusAndPools.stream()
                .flatMap(c -> c.getWindowedCount().entrySet().stream()
                        .map(windowAndCount -> toAggregatedWindowedStatistics(
                                c.getCheckType(), c.getAdvisedPool(), windowAndCount.getKey(),
                                "finished_" + c.getStatus(), windowAndCount.getValue()
                        ))
                )
                .collect(Collectors.toCollection(ArrayList::new));

        var countPoolsAggregated = finishedCountByStatusAndPools.stream().collect(
                Collectors.groupingBy(
                        c -> new CheckTypeAndStatus(c.getCheckType(), c.getStatus()),
                        Collectors.flatMapping(
                                c -> c.getWindowedCount().entrySet().stream(),
                                Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue, Long::sum)
                        )
                )
        );

        result.addAll(countPoolsAggregated.entrySet().stream()
                .flatMap(c -> c.getValue().entrySet().stream().map(
                        e -> toAggregatedWindowedStatistics(
                                c.getKey().getCheckType(), SensorLabels.ANY_POOL, e.getKey(),
                                "finished_" + c.getKey().getStatus(), e.getValue()
                        )
                ))
                .collect(Collectors.toList())
        );

        return result;
    }

    public List<AggregatedWindowedStatistics> getWindowedStartFinishIterationsCount(
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        var maxWindow = aggregateWindows.stream().max(Duration::compareTo).orElse(Duration.ZERO);
        var fromWithMaxWindow = to.minus(maxWindow.toSeconds(), ChronoUnit.SECONDS);

        var startedCountByPools = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getStartedCount(fromWithMaxWindow, to, aggregateWindows, checkTypes));

        var finishedCountByPools = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getFinishedCount(fromWithMaxWindow, to, aggregateWindows, checkTypes));

        var result = startedCountByPools.stream()
                .flatMap(
                        c -> c.getWindowedCount().entrySet().stream()
                                .map(windowAndCount -> toAggregatedWindowedStatistics(
                                        c.getCheckType(), c.getAdvisedPool(), c.getIterationType(),
                                        windowAndCount.getKey(), "started", windowAndCount.getValue()
                                ))
                )
                .collect(Collectors.toCollection(ArrayList::new));

        result.addAll(
                finishedCountByPools.stream()
                        .flatMap(c -> c.getWindowedCount().entrySet().stream()
                                .map(windowAndCount -> toAggregatedWindowedStatistics(
                                        c.getCheckType(), c.getAdvisedPool(), c.getIterationType(),
                                        windowAndCount.getKey(), "finished", windowAndCount.getValue()
                                ))
                        )
                        .collect(Collectors.toList())
        );

        result.addAll(
                groupByCheckTypeAndCheckIterationType(startedCountByPools).entrySet().stream()
                        .flatMap(groupEntry -> groupEntry.getValue().entrySet().stream().map(
                                windowAndCount -> toAggregatedWindowedStatistics(
                                        groupEntry.getKey(), SensorLabels.ANY_POOL,
                                        windowAndCount.getKey(), "started", windowAndCount.getValue()
                                )
                        ))
                        .collect(Collectors.toList())
        );

        result.addAll(
                groupByCheckTypeAndCheckIterationType(finishedCountByPools).entrySet().stream()
                        .flatMap(groupEntry -> groupEntry.getValue().entrySet().stream().map(
                                windowAndCount -> toAggregatedWindowedStatistics(
                                        groupEntry.getKey(), SensorLabels.ANY_POOL,
                                        windowAndCount.getKey(), "finished", windowAndCount.getValue()
                                )
                        ))
                        .collect(Collectors.toList())
        );

        return result;
    }

    public List<AggregatedWindowedStatistics> getWindowedStartFinishIterationsCountByCreateSystem(
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        var maxWindow = aggregateWindows.stream().max(Duration::compareTo).orElse(Duration.ZERO);
        var fromWithMaxWindow = to.minus(maxWindow.toSeconds(), ChronoUnit.SECONDS);

        var startedCount = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getStartedCountByCreateSystem(
                        fromWithMaxWindow, to, aggregateWindows, checkTypes
                ));

        var finishedCount = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getFinishedCountByCreateSystem(
                        fromWithMaxWindow, to, aggregateWindows, checkTypes
                ));

        var result = startedCount.stream()
                .flatMap(
                        c -> c.getWindowedCount().entrySet().stream()
                                .map(e -> new AggregatedWindowedStatistics(
                                        Map.of(
                                                SensorLabels.CREATE_SYSTEM, c.getCreateSystem(),
                                                SensorLabels.WINDOW, e.getKey(),
                                                SensorLabels.METRIC, "started"
                                        ),
                                        e.getValue()
                                ))
                )
                .collect(Collectors.toCollection(ArrayList::new));

        result.addAll(
                finishedCount.stream()
                        .flatMap(c -> c.getWindowedCount().entrySet().stream()
                                .map(e -> new AggregatedWindowedStatistics(
                                        Map.of(
                                                SensorLabels.CREATE_SYSTEM, c.getCreateSystem(),
                                                SensorLabels.WINDOW, e.getKey(),
                                                SensorLabels.METRIC, "finished"
                                        ),
                                        e.getValue()
                                ))
                        )
                        .collect(Collectors.toList())
        );

        return result;
    }

    public List<InflightIterationsCount> getInflightIterationsCountByCreateSystem(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull List<CheckOuterClass.CheckType> checkTypes
    ) {
        var inflightsCount = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getInflightAggregationByCreatedSystem(from, to, checkTypes));

        return inflightsCount.stream()
                .map(g -> new InflightIterationsCount(
                        Map.of(SensorLabels.CREATE_SYSTEM, g.getCreateSystem()),
                        g.getCount()
                )).collect(Collectors.toCollection(ArrayList::new));
    }

    /**
     * Calculates and saves SLA statistics for main SLA chart
     */
    public List<SlaStatisticsEntity> getSlaStatistics(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull CheckOuterClass.CheckType checkType,
            @Nonnull IterationTypeGroup iterationType,
            @Nonnull IterationCompleteGroup status,
            int windowDays,
            @Nullable String advisedPool,
            @Nonnull Set<String> authors,
            @Nullable Long totalNumberOfNodes,
            boolean recalculateSelected
    ) {
        var authorsSorted = authors.stream().sorted().collect(Collectors.joining(","));
        var lastSaveDate = clock.instant().minus(lastSaveDelay).truncatedTo(ChronoUnit.DAYS);
        var nowTime = clock.instant();
        if (to.isAfter(nowTime)) {
            to = nowTime;
        }

        var toFinal = to.truncatedTo(ChronoUnit.DAYS);
        var fromFinal = from.truncatedTo(ChronoUnit.DAYS);

        Map<Instant, SlaStatisticsEntity> fromDb = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.slaStatistics().findRange(
                        fromFinal, toFinal, checkType, iterationType, status,
                        windowDays, advisedPool, authorsSorted, totalNumberOfNodes
                )).stream()
                .collect(Collectors.toMap(e -> e.getId().getDay(), Function.identity()));

        Map<Instant, SlaStatisticsEntity> calculated = new HashMap<>();
        for (Instant day = fromFinal; !day.isAfter(toFinal); day = day.plus(1, ChronoUnit.DAYS)) {
            var existingId = fromDb.containsKey(day) ? fromDb.get(day).getId() : null;

            if (existingId != null && !recalculateSelected) {
                continue;
            }

            var calculatedDay = calculateSlaStatisticsForDay(
                    day, checkType, iterationType, status, windowDays,
                    advisedPool, authors, authorsSorted, totalNumberOfNodes,
                    existingId
            );

            calculated.put(day, calculatedDay);
            if (!calculatedDay.getId().getDay().isAfter(lastSaveDate)) {
                if (existingId != null) {
                    db.tx(() -> db.slaStatistics().save(calculatedDay));
                } else {
                    db.tx(() -> db.slaStatistics().saveIfAbsent(calculatedDay));
                }
            }
        }

        return Stream.concat(fromDb.values().stream(), calculated.values().stream())
                .sorted(Comparator.comparing(e -> e.getId().getDay()))
                .collect(Collectors.toList());
    }

    private List<AggregatedStageDuration> computeStagesDurationsPercentiles(
            String advisedPool,
            List<CheckIterationEntity> iterations,
            IterationType iterationType,
            StageType stageType,
            String checkTypeLabel
    ) {
        Map<String, List<Long>> durationSeconds = new HashMap<>();

        for (var iteration : iterations) {
            switch (iterationType) {
                case MAIN:
                    if (CheckStatusUtils.isActive(iteration.getStatus())) {
                        addStagesDurationsToMap(
                                iteration.getStagesAggregation(), iteration.getCreated(), iteration.getId(),
                                durationSeconds, stageType
                        );
                        addTotalStageToMap(iteration.getCreated(), durationSeconds);
                    }
                    break;
                case RECHECK:
                    iteration.getRecheckAggregationsByNumber().values().stream()
                            .filter(r -> CheckStatusUtils.isActive(r.getStatus()))
                            .forEach(r -> {
                                addStagesDurationsToMap(
                                        r.getStagesAggregation(), r.getCreated(), iteration.getId(),
                                        durationSeconds, stageType
                                );
                                addTotalStageToMap(r.getCreated(), durationSeconds);
                            });
                    break;
                case ANY:
                    if (CheckStatusUtils.isActive(iteration.getStatus())) {
                        addStagesDurationsToMap(
                                iteration.getStagesAggregation(), iteration.getCreated(), iteration.getId(),
                                durationSeconds, stageType
                        );
                    } else {
                        iteration.getRecheckAggregationsByNumber().values().stream()
                                .filter(r -> CheckStatusUtils.isActive(r.getStatus()))
                                .forEach(r -> addStagesDurationsToMap(
                                        r.getStagesAggregation(), r.getCreated(), iteration.getId(),
                                        durationSeconds, stageType
                                ));
                    }
                    addTotalStageToMap(iteration.getCreated(), durationSeconds);
                    break;
                default:
                    log.warn("Unknown iteration type {} in aggregated metrics", iterationType);
                    break;
            }
        }

        addZeroesToStageIfAbsent(TOTAL_STAGE, durationSeconds);
        for (var stage : stagesToAggregate) {
            addZeroesToStageIfAbsent(stage, durationSeconds);
        }

        return durationSeconds.entrySet().stream()
                .flatMap(
                        e -> percentiles.compute(e.getValue()).entrySet().stream()
                                .map(p -> new AggregatedStageDuration(
                                        Map.of(
                                                SensorLabels.PERCENTILE, p.getKey().toString(),
                                                SensorLabels.STAGE, e.getKey(),
                                                SensorLabels.POOL, advisedPool,
                                                SensorLabels.ITERATION, iterationType.toString(),
                                                SensorLabels.STAGE_TYPE, stageType.toString(),
                                                SensorLabels.CHECK_TYPE, checkTypeLabel
                                        ),
                                        p.getValue()
                                ))
                )
                .collect(Collectors.toList());
    }

    private void addStagesDurationsToMap(
            DurationStages durationStages, Instant created, CheckIterationEntity.Id iterationId,
            Map<String, List<Long>> durationSeconds, StageType stageType
    ) {
        long accumulatedStagesSeconds = 0;
        boolean incompletedStageFound = false;

        for (var stage : stagesToAggregate) {
            if (!incompletedStageFound) {
                var stageDuration = getStageDuration(
                        stage, durationStages, created, iterationId, accumulatedStagesSeconds
                );

                if (stageDuration.isCompleted()) {
                    accumulatedStagesSeconds += stageDuration.getSeconds();
                    if (stageType != StageType.ACTIVE) {
                        durationSeconds.computeIfAbsent(stage, k -> new ArrayList<>()).add(stageDuration.getSeconds());
                    }
                } else {
                    durationSeconds.computeIfAbsent(stage, k -> new ArrayList<>()).add(stageDuration.getSeconds());
                    incompletedStageFound = true;
                }
            } else if (stageType == StageType.WITH_ZEROES) {
                durationSeconds.computeIfAbsent(stage, k -> new ArrayList<>()).add(0L);
            } else {
                return;
            }
        }
    }

    private void addTotalStageToMap(Instant created, Map<String, List<Long>> durationSeconds) {
        var duration = Math.max(0, clock.instant().minusSeconds(created.getEpochSecond()).getEpochSecond());

        durationSeconds.computeIfAbsent(TOTAL_STAGE, k -> new ArrayList<>()).add(duration);
    }

    private void addZeroesToStageIfAbsent(String stage, Map<String, List<Long>> durationSeconds) {
        if (!durationSeconds.containsKey(stage)) {
            durationSeconds.put(stage, List.of(0L));
        }
    }

    private DurationStages.StageDuration getStageDuration(
            String stage, DurationStages durationStages, Instant created,
            CheckIterationEntity.Id iterationId, long accumulatedStagesSeconds
    ) {
        if (durationStages.getStageDuration(stage).isCompleted()) {
            return durationStages.getStageDuration(stage);
        }

        var durationSeconds = clock.instant()
                .minusSeconds(created.getEpochSecond())
                .minusSeconds(accumulatedStagesSeconds)
                .getEpochSecond();

        if (durationSeconds < 0) {
            log.warn("Negative stage {} duration {} for iteration {}", stage, durationSeconds, iterationId);
        }

        return new DurationStages.StageDuration(Math.max(0, durationSeconds), false);
    }

    private Map<CheckTypeAndCheckIterationType, Map<String, Long>> groupByCheckTypeAndCheckIterationType(
            List<WindowedStatsByPoolCount> windowedCountsByPools
    ) {
        return windowedCountsByPools.stream().collect(Collectors.groupingBy(
                CheckTypeAndCheckIterationType::of,
                Collectors.flatMapping(
                        c -> c.getWindowedCount().entrySet().stream(),
                        Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue, Long::sum)
                )
        ));
    }

    private AggregatedWindowedStatistics toAggregatedWindowedStatistics(
            CheckOuterClass.CheckType checkType, String advisedPool,
            String window, String metric, Long value
    ) {
        return new AggregatedWindowedStatistics(
                Map.of(
                        SensorLabels.CHECK_TYPE, checkType.toString(),
                        SensorLabels.POOL, advisedPool,
                        SensorLabels.WINDOW, window,
                        SensorLabels.METRIC, metric
                ),
                value
        );
    }

    private AggregatedWindowedStatistics toAggregatedWindowedStatistics(
            CheckTypeAndCheckIterationType checkTypeAndCheckIterationType, String advisedPool,
            String window, String metric, Long value
    ) {
        return toAggregatedWindowedStatistics(
                checkTypeAndCheckIterationType.getCheckType(),
                advisedPool,
                checkTypeAndCheckIterationType.getIterationType(),
                window, metric, value
        );
    }

    private AggregatedWindowedStatistics toAggregatedWindowedStatistics(
            CheckOuterClass.CheckType checkType, String advisedPool, CheckIteration.IterationType iterationType,
            String window, String metric, Long value
    ) {
        return new AggregatedWindowedStatistics(
                Map.of(
                        SensorLabels.CHECK_TYPE, checkType.toString(),
                        SensorLabels.POOL, advisedPool,
                        SensorLabels.CHECK_ITERATION_TYPE, iterationType.toString(),
                        SensorLabels.WINDOW, window,
                        SensorLabels.METRIC, metric
                ),
                value
        );
    }

    private List<CheckIterationEntity> getActiveTrunkPrecommitIterations(
            Instant from,
            Instant to,
            List<CheckOuterClass.CheckType> checkTypes,
            boolean includeStressTestChecks
    ) {
        List<YqlStatementPart<?>> filters = new ArrayList<>();
        filters.add(YqlPredicate.or(
                YqlPredicate.where("id.iterType").eq(CheckIteration.IterationType.FAST),
                YqlPredicate.where("id.iterType").eq(CheckIteration.IterationType.FULL)
        ));
        filters.add(YqlPredicate.or(
                CheckStatusUtils.getRunning("status"),
                YqlPredicate.where("hasUnfinishedRechecks").eq(true)
        ));
        filters.add(YqlPredicate.where("parentIterationNumber").isNull());
        filters.add(YqlPredicate.where("pessimized").eq(false));

        if (!checkTypes.isEmpty()) {
            filters.add(YqlPredicate.where("checkType").in(checkTypes));
        }

        if (!includeStressTestChecks) {
            filters.add(
                    YqlPredicate.where("stressTest").isNull()
                            .or(YqlPredicate.where("stressTest").eq(false))
            );
        }

        return db.readOnly(() -> db.iterations().findByCreated(from, to, filters));
    }

    private SlaStatisticsEntity calculateSlaStatisticsForDay(
            Instant day,
            CheckOuterClass.CheckType checkType,
            IterationTypeGroup iterationType,
            IterationCompleteGroup status,
            int windowDays,
            @Nullable String advisedPool,
            Set<String> authors,
            String authorsSortedString,
            @Nullable Long totalNumberOfNodes,
            @Nullable SlaStatisticsEntity.Id existingId
    ) {
        List<YqlPredicate> filters = new ArrayList<>();
        filters.add(YqlPredicate.where("checkType").eq(checkType));
        filters.add(YqlPredicate.where("status").in(status.getStatuses()));
        filters.add(YqlPredicate.or(
                iterationType.getIterationTypes().stream()
                        .map(t -> YqlPredicate.where("id.iterType").eq(t))
                        .collect(Collectors.toList())
        ));

        if (advisedPool != null) {
            filters.add(YqlPredicate.where("advisedPool").eq(advisedPool));
        }
        if (!authors.isEmpty()) {
            filters.add(YqlPredicate.where("author").in(authors));
        }
        if (totalNumberOfNodes != null) {
            filters.add(YqlPredicate.where("statistics.totalNumberOfNodes").lte(totalNumberOfNodes));
        }

        var fromDb = db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.iterations().getWindowedSlaStatisticsForDay(day, windowDays, percentileIndexes, filters));

        var slaStatisticsId = existingId == null
                ? SlaStatisticsEntity.Id.create(checkType, iterationType, status, windowDays, day)
                : existingId;

        return SlaStatisticsEntity.builder()
                .id(slaStatisticsId)
                .durationSecondsPercentiles(
                        Objects.requireNonNullElse(fromDb.getDurationSecondsPercentiles(), Map.of())
                )
                .avgIterationNumber(fromDb.getAvgIterationNumber())
                .avgIterationNumberWithRobots(fromDb.getAvgIterationNumberWithRobots())
                .advisedPool(advisedPool)
                .authors(authorsSortedString)
                .totalNumberOfNodes(totalNumberOfNodes)
                .build();
    }

    private enum IterationType {
        MAIN("main"),
        RECHECK("recheck"),
        ANY("any");

        private final String label;

        IterationType(String label) {
            this.label = label;
        }

        @Override
        public String toString() {
            return label;
        }
    }

    @Value
    private static class CheckTypeAndStatus {
        CheckOuterClass.CheckType checkType;
        Common.CheckStatus status;
    }

}
