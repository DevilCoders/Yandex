package ru.yandex.ci.storage.core.db.model.test_revision;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.statement.NumberOfRevisionsStatement;
import ru.yandex.ci.storage.core.db.model.test_revision.statement.TestRevisionStatements;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class TestRevisionTable extends KikimrTableCi<TestRevisionEntity> {
    public TestRevisionTable(KikimrTable.QueryExecutor executor) {
        super(TestRevisionEntity.class, executor);
    }

    public List<TestRevisionEntity> getRevisions(
            TestStatusEntity.Id testId, long fromRevision, long toRevision
    ) {
        return this.readTable(
                ReadTableParams.<TestRevisionEntity.Id>builder()
                        .fromKey(new TestRevisionEntity.Id(testId, fromRevision))
                        .toKey(new TestRevisionEntity.Id(testId, toRevision))
                        .fromInclusive(true)
                        .toInclusive(false)
                        .ordered()
                        .build()
        ).collect(Collectors.toList());
    }

    public List<TestRevisionEntity> loadOnRevisions(
            TestStatusEntity.Id statusId, Collection<Long> revisions
    ) {
        var predicate = YqlPredicate
                .where("id.statusId.testId").eq(statusId.getTestId())
                .and("id.statusId.suiteId").eq(statusId.getSuiteId())
                .and("id.statusId.branch").eq(statusId.getBranch())
                .and("id.revision").in(revisions);

        if (!statusId.getToolchain().isEmpty() && !statusId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            predicate = predicate.and("id.statusId.toolchain").eq(statusId.getToolchain());
        }

        return this.find(
                predicate,
                YqlOrderBy.orderBy(
                        "id.statusId.testId", "id.statusId.suiteId", "id.statusId.branch",
                        "id.revision", "id.statusId.toolchain"
                )
        );
    }

    public Optional<TestRevisionEntity> getLast(TestStatusEntity.Id statusId, HistoryFilters filters) {
        var predicate = YqlPredicate
                .where("id.statusId.testId").eq(statusId.getTestId())
                .and("id.statusId.suiteId").eq(statusId.getSuiteId())
                .and("id.statusId.branch").eq(statusId.getBranch());

        if (!statusId.getToolchain().isEmpty() && !statusId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            predicate = predicate.and("id.statusId.toolchain").eq(statusId.getToolchain());
        }

        var from = Math.max(filters.getFromRevision(), filters.getToRevision());
        var to = Math.min(filters.getFromRevision(), filters.getToRevision());

        if (from > 0) {
            predicate = predicate.and("id.revision").lte(from);
        }

        if (to > 0) {
            predicate = predicate.and("id.revision").gte(to);
        }

        var status = filters.getStatus();
        if (!status.equals(Common.TestStatus.UNRECOGNIZED) && !status.equals(Common.TestStatus.TS_UNKNOWN)) {
            predicate = predicate.and(YqlPredicate.where("status").eq(status));
        }

        var results = this.find(
                predicate,
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.statusId.testId", YqlOrderBy.SortOrder.DESC),
                        new YqlOrderBy.SortKey("id.statusId.suiteId", YqlOrderBy.SortOrder.DESC),
                        new YqlOrderBy.SortKey("id.statusId.branch", YqlOrderBy.SortOrder.DESC),
                        new YqlOrderBy.SortKey("id.revision", YqlOrderBy.SortOrder.DESC)
                ),
                YqlLimit.top(1)
        );

        return results.isEmpty() ? Optional.empty() : Optional.of(results.get(0));
    }

    public Map<WrappedRevisionsBoundaries, Long> countRevisions(
            TestStatusEntity.Id testId,
            HistoryFilters filters,
            Collection<WrappedRevisionsBoundaries> values
    ) {
        var boundaries = values.stream().collect(
                Collectors.toMap(WrappedRevisionsBoundaries::generateName, Function.identity())
        );

        var result = executeOnView(new NumberOfRevisionsStatement(testId, filters, boundaries), null);

        return result.stream().collect(
                Collectors.toMap(
                        r -> boundaries.get(r.getBoundary()), NumberOfRevisionsStatement.NumberOfRevisions::getNumber
                )
        );
    }

    public List<Long> getWrappedRevisions(
            TestStatusEntity.Id statusId,
            HistoryFilters filters,
            WrappedRevisionsBoundaries boundaries,
            int limit
    ) {
        var predicate = YqlPredicate
                .where("id.statusId.testId").eq(statusId.getTestId())
                .and("id.statusId.suiteId").eq(statusId.getSuiteId())
                .and("id.statusId.branch").eq(statusId.getBranch());

        if (!statusId.getToolchain().isEmpty() && !statusId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            predicate = predicate.and("id.statusId.toolchain").eq(statusId.getToolchain());
        }

        predicate = predicate
                .and("id.revision").lt(boundaries.getFrom())
                .and("id.revision").gt(boundaries.getTo());

        var status = filters.getStatus();
        if (!status.equals(Common.TestStatus.UNRECOGNIZED) && !status.equals(Common.TestStatus.TS_UNKNOWN)) {
            predicate = predicate.and(YqlPredicate.where("previousStatus").eq(status).or("status").eq(status));
        }

        return this.findDistinct(
                TestRevisionEntity.RevisionView.class,
                List.of(
                        predicate,
                        YqlOrderBy.orderBy(
                                new YqlOrderBy.SortKey("id.statusId.testId", YqlOrderBy.SortOrder.DESC),
                                new YqlOrderBy.SortKey("id.statusId.suiteId", YqlOrderBy.SortOrder.DESC),
                                new YqlOrderBy.SortKey("id.statusId.branch", YqlOrderBy.SortOrder.DESC),
                                new YqlOrderBy.SortKey("id.revision", YqlOrderBy.SortOrder.DESC)
                        ),
                        YqlLimit.top(limit)
                )
        ).stream().map(x -> x.getId().getRevision()).collect(Collectors.toList());
    }

    public Collection<Integer> getRegions(TestStatusEntity.Id testId, FragmentationSettings settings) {
        var predicate = YqlPredicate.where("id.statusId").eq(testId);
        var query = TestRevisionStatements.createRegionsStatement(
                predicate,
                settings.getRevisionsInRegions()
        );

        return this.executor.execute(query, List.of(predicate)).stream()
                .map(x -> (int) x.getRevision())
                .collect(Collectors.toList());
    }

    public List<Integer> getBuckets(TestStatusEntity.Id testId, int region, FragmentationSettings settings) {
        var regionOffset = region * settings.getRevisionsInRegions();
        var predicate = YqlPredicate.where("id.statusId").eq(testId)
                .and("id.revision").gte(regionOffset)
                .and("id.revision").lt(regionOffset + settings.getRevisionsInRegions());
        var query = TestRevisionStatements.createBucketsStatement(
                predicate,
                regionOffset,
                settings.getRevisionsInBucket()
        );

        return this.executor.execute(query, List.of(predicate)).stream()
                .map(x -> (int) x.getRevision())
                .collect(Collectors.toList());
    }
}
