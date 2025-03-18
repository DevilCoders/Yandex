package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.util.ObjectStore;

@Slf4j
public class TestDiffTable extends DiffTable<TestDiffEntity> {
    public TestDiffTable(QueryExecutor executor) {
        super(TestDiffEntity.class, executor);
    }

    public List<TestDiffEntity> listSuite(
            TestDiffEntity.Id diffId, SuiteSearchFilters filters, int skip, int take
    ) {
        final var filter = new ObjectStore<>(
                YqlPredicate
                        .where("id.checkId").eq(diffId.getCheckId())
                        .and("id.iterationType").eq(diffId.getIterationType())
                        .and("id.toolchain").eq(diffId.getToolchain())
                        .and("id.suiteId").eq(diffId.getSuiteId())
                        .and("id.testId").neq(diffId.getSuiteId())
        );

        SearchDiffQueries.fillIterationFilter(diffId.getIterationId(), filter);
        SearchDiffQueries.fillResultTypesFilter(filter, Set.of(ResultTypeUtils.toChildType(diffId.getResultType())));
        SearchDiffQueries.fillCommonFiltersWithPath(filters, filter);

        if (!filters.getTestName().isEmpty()) {
            filter.set(filter.get().and(YqlPredicate.where("name").eq(filters.getTestName())));
        }

        if (!filters.getSubtestName().isEmpty()) {
            filter.set(filter.get().and(YqlPredicate.where("subtestName").eq(filters.getSubtestName())));
        }

        return this.find(
                filter.get(),
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.path", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("name", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("subtestName", YqlOrderBy.SortOrder.ASC)
                ),
                YqlLimit.range(skip, take)
        );
    }

    public List<TestDiffEntity> listDiff(
            TestDiffEntity.Id diffId,
            Set<String> allToolchains,
            int numberOfIterations
    ) {
        /* todo use this after Ydb uses index with IN request
        var predicate = YqlPredicate
                .where("id.checkId").eq(diffId.getCheckId())
                .and("id.iterationType").eq(diffId.getIterationType())
                .and("id.suiteId").eq(diffId.getSuiteId())
                .and("id.toolchain").in(allToolchains)
                .and("id.testId").eq(diffId.getTestId());

        final var filter = new ObjectStore<>(predicate);

        fillIterationFilter(diffId.getIterationId(), filter);
        fillResultTypesFilter(filter, Set.of(diffId.getResultType()));
        fillNotAggregateToolchainFilter(filter, diffId.getToolchain());
        fillCommonFilters(filters, filter);

        return this.find(filter.get(), YqlOrderBy.orderBy("id.toolchain"));
         */

        if (diffId.getIterationNumber() == 0) {
            var ids = new HashSet<TestDiffEntity.Id>(allToolchains.size() * numberOfIterations + 1);
            for (int i = 0; i <= numberOfIterations; ++i) {
                for (var toolchain : allToolchains) {
                    ids.add(alterDiffId(diffId, i, toolchain));
                }
            }

            ids.add(diffId);

            return this.find(ids).stream()
                    .filter(diff -> diff.isLast() || diff.getId().equals(diffId))
                    .collect(Collectors.toList());
        } else {
            var ids = new HashSet<TestDiffEntity.Id>(allToolchains.size());
            for (var toolchain : allToolchains) {
                ids.add(alterDiffId(diffId, diffId.getIterationNumber(), toolchain));
            }

            ids.add(diffId);

            return this.find(ids);
        }
    }

    private TestDiffEntity.Id alterDiffId(TestDiffEntity.Id diffId, int iterationNumber, String toolchain) {
        return new TestDiffEntity.Id(
                diffId.getCheckId(),
                diffId.getIterationType(),
                diffId.getResultType(),
                toolchain,
                diffId.getPath(),
                diffId.getSuiteId(),
                diffId.getTestId(),
                iterationNumber
        );
    }

    public List<TestDiffEntity> searchDiffs(
            CheckIterationEntity.Id iterationId, DiffSearchFilters filters, int skip, int take
    ) {
        var filter = SearchDiffQueries.getSearchDiffsFilter(iterationId, filters);

        log.info("Search diffs, filter: {}, skip: {}, take: {}", filter.get(), skip, take);

        return this.find(
                filter.get(),
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.resultType", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("id.path", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("id.toolchain", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("name", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("subtestName", YqlOrderBy.SortOrder.ASC)
                ),
                YqlLimit.range(skip, skip + take)
        );
    }

    public List<TestDiffEntity> searchAllDiffs(
            CheckIterationEntity.Id iterationId,
            DiffSearchFilters filters
    ) {
        var filter = SearchDiffQueries.getSearchDiffsFilter(iterationId, filters);
        log.info("Search all diffs, filter: {}", filter.get());
        return this.find(filter.get());
    }

    public List<String> listToolchains(CheckIterationEntity.Id iterationId, DiffSearchFilters filters) {
        var filter = SearchDiffQueries.getSearchDiffsFilter(iterationId, filters);
        log.info("Search toolchains, filter: {}", filter.get());

        return this.findIds(filter.get()).stream()
                .map(id -> (TestDiffEntity.Id) id)
                .map(TestDiffEntity.Id::getToolchain)
                .distinct()
                .toList();
    }

    public Stream<TestDiffEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = TestDiffEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MIN_VALUE)
                .build();
        var toId = TestDiffEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MAX_VALUE)
                .build();

        return this.readTableIds(
                ReadTableParams.<TestDiffEntity.Id>builder()
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
