package ru.yandex.ci.observer.core.db.model.check_iterations;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.utils.ObserverStatisticConstants;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

public class CheckIterationsTable extends KikimrTableCi<CheckIterationEntity> {
    public CheckIterationsTable(QueryExecutor executor) {
        super(CheckIterationEntity.class, executor);
    }

    @Nullable
    public CheckIterationEntity findLast(CheckEntity.Id id, CheckIteration.IterationType iterationType) {
        return this.find(
                YqlPredicate.where("id.checkId").eq(id).and("id.type").eq(iterationType)
        ).stream().max(Comparator.comparingInt(a -> a.getId().getNumber())).orElse(null);
    }

    public long countActive() {
        return this.count(
                CheckStatusUtils.getIsActive("status"),
                byStatus()
        );
    }

    public long countActive(CheckEntity.Id id) {
        return this.count(
                YqlPredicate.and(
                        YqlPredicate.where("id.checkId").eq(id),
                        CheckStatusUtils.getIsActive("status")
                )
        );
    }

    public List<CheckIterationEntity> findActive(CheckEntity.Id id) {
        return this.find(
                YqlPredicate.and(
                        YqlPredicate.where("id.checkId").eq(id),
                        CheckStatusUtils.getIsActive("status")
                )
        );
    }

    public List<CheckIterationEntity> findRunningStartedBefore(Instant beforeTime, int limit) {
        return this.find(
                YqlPredicate.and(
                        CheckStatusUtils.getIsActive("status"),
                        YqlPredicate.where("created").lte(beforeTime)
                ),
                YqlLimit.range(0, limit),
                byStatus()
        );
    }

    public List<CheckIterationEntity> findByChecks(Set<CheckEntity.Id> ids) {
        return this.find(YqlPredicate.where("id.checkId").in(ids));
    }

    public List<CheckIterationEntity> findByCheck(CheckEntity.Id id) {
        return this.find(YqlPredicate.where("id.checkId").eq(id));
    }

    public List<CheckIterationEntity> findByCreated(
            Instant createdFrom, Instant createdTo,
            List<YqlStatementPart<?>> additionalFilters
    ) {
        return find(Stream.concat(
                Stream.of(
                        YqlPredicate.where("created").gte(createdFrom).and("created").lte(createdTo),
                        YqlOrderBy.orderBy("created", "id"),
                        YqlView.index(CheckIterationEntity.IDX_BY_CREATED)
                ),
                additionalFilters.stream()
        ).collect(Collectors.toList()));
    }

    public List<CheckIterationEntity> findByRightRevisionTimestamp(
            Instant from, Instant to,
            List<YqlStatementPart<?>> additionalFilters
    ) {
        return find(Stream.concat(
                Stream.of(
                        YqlPredicate.where("rightRevisionTimestamp").gte(from).and("rightRevisionTimestamp").lte(to),
                        YqlOrderBy.orderBy("rightRevisionTimestamp", "id"),
                        YqlView.index(CheckIterationEntity.IDX_BY_RIGHT_REVISION_TIMESTAMP)
                ),
                additionalFilters.stream()
        ).collect(Collectors.toList()));
    }

    public List<CheckIterationEntity> scanByCreated(
            Instant createdFrom, Instant createdTo,
            List<YqlStatementPart<?>> additionalFilters
    ) {
        return find(Stream.concat(
                Stream.of(
                        YqlPredicate.where("created").gte(createdFrom).and("created").lte(createdTo),
                        YqlOrderBy.orderBy("created")
                ),
                additionalFilters.stream()
        ).collect(Collectors.toList()));
    }

    public List<CountByCheckTypeAdvisedPoolIterationType> getInflightAggregation(
            Instant createdFrom, Instant createdTo, List<CheckOuterClass.CheckType> checkTypes
    ) {
        List<YqlStatementPart<?>> filterStatement = getInflightCountStatement(createdFrom, createdTo, checkTypes);
        return this.groupBy(
                CountByCheckTypeAdvisedPoolIterationType.class, List.of("checkType", "advisedPool", "id_iterType"),
                filterStatement
        );
    }

    public List<CountByCreateSystem> getInflightAggregationByCreatedSystem(
            Instant createdFrom, Instant createdTo, List<CheckOuterClass.CheckType> checkTypes
    ) {
        List<YqlStatementPart<?>> filterStatement = getInflightCountStatement(createdFrom, createdTo, checkTypes);

        return this.groupBy(
                CountByCreateSystem.class,
                List.of("createSystem"),
                List.of("IF((testenvId is not NULL) and (testenvId != ''), 'TestEnv', 'CI') as createSystem"),
                filterStatement
        );
    }

    public List<WindowedStatsByCreateSystem> getStartedCountByCreateSystem(
            Instant from, Instant to,
            List<Duration> windows,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        return getWindowedCounts(
                WindowedStatsByCreateSystem.class,
                from, to, windows,
                List.of("createSystem"),
                "created", CheckIterationEntity.IDX_BY_CREATED,
                checkTypes, List.of(),
                List.of("IF((testenvId is not NULL) and (testenvId != ''), 'TestEnv', 'CI') as createSystem")
        );
    }

    public List<WindowedStatsByCreateSystem> getFinishedCountByCreateSystem(
            Instant from, Instant to,
            List<Duration> windows,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        return getWindowedCounts(
                WindowedStatsByCreateSystem.class,
                from, to, windows,
                List.of("createSystem"),
                "finish", CheckIterationEntity.IDX_BY_FINISH,
                checkTypes, List.of(),
                List.of("IF((testenvId is not NULL) and (testenvId != ''), 'TestEnv', 'CI') as createSystem")
        );
    }

    public List<WindowedStatsByPoolCount> getStartedCount(
            Instant from, Instant to,
            List<Duration> windows,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        return getWindowedCounts(
                WindowedStatsByPoolCount.class,
                from, to, windows, List.of("checkType", "advisedPool", "id_iterType"),
                "created", CheckIterationEntity.IDX_BY_CREATED,
                checkTypes, List.of(), List.of("checkType", "advisedPool", "id_iterType")
        );
    }

    public List<WindowedStatsByPoolCount> getFinishedCount(
            Instant from, Instant to,
            List<Duration> windows,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        return getWindowedCounts(
                WindowedStatsByPoolCount.class,
                from, to, windows, List.of("checkType", "advisedPool", "id_iterType"),
                "finish", CheckIterationEntity.IDX_BY_FINISH,
                checkTypes, List.of(), List.of("checkType", "advisedPool", "id_iterType")
        );
    }

    public List<WindowedStatsByStatusAndPoolCount> getFinishedStatusCount(
            Instant from, Instant to,
            List<Duration> windows,
            List<CheckOuterClass.CheckType> checkTypes
    ) {
        return getWindowedCounts(
                WindowedStatsByStatusAndPoolCount.class,
                from, to, windows, List.of("checkType", "advisedPool", "status"),
                "finish", CheckIterationEntity.IDX_BY_FINISH,
                checkTypes,
                List.of(YqlPredicate.where("status").in(
                        Common.CheckStatus.COMPLETED_SUCCESS, Common.CheckStatus.COMPLETED_FAILED,
                        Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR
                )),
                List.of("checkType", "advisedPool", "status")
        );
    }

    public AggregatedSlaStats getWindowedSlaStatisticsForDay(
            Instant day,
            int windowDays,
            List<Integer> percentileIndexes,
            List<YqlPredicate> filters
    ) {
        var to = day.plus(1, ChronoUnit.DAYS); //including parameter day
        var from = to.minus(windowDays, ChronoUnit.DAYS);
        var percentilesColumn = toYqlDictPercentileColumn(
                percentileIndexes, "totalDurationSeconds", "durationSecondsPercentiles"
        );

        List<YqlStatementPart<?>> allFilters = new ArrayList<>(filters);
        allFilters.add(YqlPredicate.where("created").gte(from).and("created").lt(to));
        allFilters.add(YqlPredicate.where("parentIterationNumber").isNull());
        allFilters.add(YqlPredicate.where("hasUnfinishedRechecks").eq(false));
        allFilters.add(YqlPredicate.where("totalDurationSeconds").isNotNull());

        return aggregate(
                AggregatedSlaStats.class,
                List.of(
                        percentilesColumn,
                        "COUNT(*) / %S AS avgIterationNumberWithRobots".formatted(windowDays),
                        "COUNT_IF((find(author, 'robot-') is NULL AND find(author, '-robot') is NULL)) /" +
                                " %s AS avgIterationNumber".formatted(windowDays)
                ),
                allFilters
        ).orElseThrow();
    }

    private <T extends View> List<T> getWindowedCounts(
            Class<T> viewClass,
            Instant from, Instant to,
            List<Duration> windows,
            List<String> selectColumns,
            String column,
            String index,
            List<CheckOuterClass.CheckType> checkTypes,
            List<YqlPredicate> additionalFilters,
            List<String> groupColumns
    ) {
        var countColumn = toYqlDictWindowCountColumn(windows, to, column, "windowedCount");

        List<YqlStatementPart<?>> filterStatement = List.of(
                YqlPredicate.where(column).gte(from).and(column).lte(to)
                        .and(YqlPredicate.or(
                                ObserverStatisticConstants.ITERATION_TYPES_USED_IN_STATISTICS
                                        .stream()
                                        .map(it -> YqlPredicate.where("id.iterType").eq(it))
                                        .toList()
                        )),
                YqlPredicate.and(additionalFilters),
                YqlPredicate.where("checkType").in(checkTypes),
                YqlView.index(index)
        );

        return this.groupBy(
                viewClass,
                Stream.concat(selectColumns.stream(), Stream.of(countColumn)).collect(Collectors.toList()),
                groupColumns,
                filterStatement
        );
    }

    private YqlView byStatus() {
        return YqlView.index(CheckIterationEntity.IDX_BY_STATUS);
    }

    private String toYqlDictWindowCountColumn(
            List<Duration> windows, Instant to, String ifColumnName, String columnName
    ) {
        return "CAST(('{" + windows.stream().map(
                w -> String.format(
                        "\"%s\": ' || CAST(COUNT_IF(`%s` >= Timestamp('%s')) AS String) || '",
                        w, ifColumnName, to.minusSeconds(w.toSeconds())
                )
        ).collect(Collectors.joining(", ")) + "}') AS Json) AS " + columnName;
    }

    private String toYqlDictPercentileColumn(
            List<Integer> percentileIndexes, String columnNameToAggregate, String aggregatedColumnName
    ) {
        return "CAST(('{" + percentileIndexes.stream().map(
                p -> String.format(
                        "\"%s\": ' || CAST(Math::Round(PERCENTILE(%s, %s)) AS String) || '",
                        p, columnNameToAggregate, p / 100.0
                )
        ).collect(Collectors.joining(", ")) + "}') AS Json) AS " + aggregatedColumnName;
    }

    private List<YqlStatementPart<?>> getInflightCountStatement(
            Instant createdFrom, Instant createdTo, List<CheckOuterClass.CheckType> checkTypes
    ) {
        return List.of(
                YqlPredicate.where("created").gte(createdFrom).and("created").lte(createdTo),
                YqlPredicate.or(
                        ObserverStatisticConstants.ITERATION_TYPES_USED_IN_STATISTICS
                                .stream()
                                .map(it -> YqlPredicate.where("id.iterType").eq(it))
                                .toList()
                ),
                YqlPredicate.where("checkType").in(checkTypes),
                CheckStatusUtils.getRunning("status"),
                YqlView.index(CheckIterationEntity.IDX_BY_CREATED)
        );
    }

    @Value
    public static class CountByCheckTypeAdvisedPoolIterationType implements View {
        CheckOuterClass.CheckType checkType;
        String advisedPool;
        @Column(name = "id_iterType")
        CheckIteration.IterationType iterationType;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;
    }

    @Value
    public static class CountByCreateSystem implements View {
        @Column(name = "createSystem", dbType = DbType.STRING)
        String createSystem;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;
    }

    @Value
    public static class WindowedStatsByCreateSystem implements View {
        @Column(name = "createSystem", dbType = DbType.STRING)
        String createSystem;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;

        @Column(name = "windowedCount")
        Map<String, Long> windowedCount;
    }

    @Value
    public static class WindowedStatsByPoolCount implements View {
        CheckOuterClass.CheckType checkType;
        String advisedPool;
        @Column(name = "id_iterType")
        CheckIteration.IterationType iterationType;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;

        @Column(name = "windowedCount")
        Map<String, Long> windowedCount;
    }

    @Value
    public static class WindowedStatsByStatusAndPoolCount implements View {
        CheckOuterClass.CheckType checkType;
        String advisedPool;
        Common.CheckStatus status;

        @Column(name = "count", dbType = DbType.UINT64)
        long count;

        @Column(name = "windowedCount")
        Map<String, Long> windowedCount;
    }

    @Value
    public static class AggregatedSlaStats implements View {
        @Column(name = "durationSecondsPercentiles")
        Map<Integer, Long> durationSecondsPercentiles;

        @Column(name = "avgIterationNumber", dbType = DbType.UINT64)
        long avgIterationNumber;

        @Column(name = "avgIterationNumberWithRobots", dbType = DbType.UINT64)
        long avgIterationNumberWithRobots;
    }
}
