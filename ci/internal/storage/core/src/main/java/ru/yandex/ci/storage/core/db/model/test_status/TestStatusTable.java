package ru.yandex.ci.storage.core.db.model.test_status;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.storage.core.utils.TextFilter;

import static ru.yandex.ci.storage.api.StorageFrontApi.NotificationFilter.NF_MUTED;
import static ru.yandex.ci.storage.api.StorageFrontApi.NotificationFilter.NF_NONE;

public class TestStatusTable extends KikimrTableCi<TestStatusEntity> {

    public TestStatusTable(QueryExecutor executor) {
        super(TestStatusEntity.class, executor);
    }

    public List<TestStatusEntity> find(long testId, long suiteId, String branch) {
        return find(
                YqlPredicate
                        .where("id.testId").eq(testId)
                        .and("id.suiteId").eq(suiteId)
                        .and("id.branch").eq(branch)
        );
    }

    public List<TestStatusEntity> find(String oldId) {
        return find(YqlPredicate.where("oldTestId").eq(oldId), YqlView.index(TestStatusEntity.IDX_OLD_ID));
    }

    public Map<TestStatusEntity.Id, List<TestStatusEntity>> search(TestSearch search) {
        var predicate = YqlPredicate.where("id.branch").eq(search.getBranch());

        if (!search.getProject().isEmpty()) {
            predicate = predicate.and("service").eq(search.getProject());
        }

        if (!search.getPath().isEmpty()) {
            predicate = predicate.and(TextFilter.get("path", search.getPath()));
        }

        if (!search.getName().isEmpty()) {
            predicate = predicate.and(TextFilter.get("name", search.getName()));
        }

        if (!search.getSubtestName().isEmpty()) {
            predicate = predicate.and(TextFilter.get("subtestName", search.getSubtestName()));
        }

        if (!search.getStatuses().isEmpty()) {
            predicate = predicate.and(YqlPredicateCi.in("status", search.getStatuses()));
        }

        if (!search.getResultTypes().isEmpty()) {
            predicate = predicate.and(YqlPredicateCi.in("type", search.getResultTypes()));
        }

        var paging = search.getPaging().getPage().getPath().isEmpty() ? null : search.getPaging();

        if (paging != null) {
            if (paging.isAscending()) {
                predicate = predicate.and(
                        YqlPredicate.where("path").eq(paging.getPage().getPath())
                                .and("id.testId").gte(paging.getPage().getTestId())
                                .or("path").gte(paging.getPage().getPath())
                );
            } else {
                predicate = predicate.and(
                        YqlPredicate.where("path").eq(paging.getPage().getPath())
                                .and("id.testId").lt(paging.getPage().getTestId())
                                .or("path").lt(paging.getPage().getPath())
                );
            }
        }

        if (!search.getNotificationFilter().equals(NF_NONE)) {
            if (search.getNotificationFilter().equals(NF_MUTED)) {
                predicate = predicate.and("muted").eq(true);
            } else {
                predicate = predicate.and("muted").eq(false);
            }
        }

        var from = 0;
        var batchSize = 128;
        var rows = new ArrayList<TestStatusEntity>();
        var byTest = Map.<TestStatusEntity.Id, List<TestStatusEntity>>of();
        while (true) {
            var statements = new ArrayList<>(selectIndexAndOrder(search, paging == null || paging.isAscending()));
            statements.add(YqlLimit.range(from, from + batchSize));
            statements.add(predicate);

            var current = this.find(statements);

            /* optimized
            var indexAndOrderBy = selectIndexAndOrder(search);
            var query = TestStatusStatements.search(
                    indexAndOrderBy.getLeft(),
                    predicate,
                    indexAndOrderBy.getRight(),
                    YqlLimit.range(from, from + batchSize),
                    paging
            );
            var current = this.executor.execute(query, List.of(predicate));
             */
            rows.addAll(current);
            byTest = rows.stream().collect(
                    Collectors.groupingBy(x -> x.getId().toAllToolchains(), LinkedHashMap::new, Collectors.toList())
            );
            if (byTest.size() > search.getPaging().getPageSize() || current.size() < batchSize) {
                break;
            }

            from += batchSize;
        }

        return byTest;
    }

    private List<YqlStatementPart<?>> selectIndexAndOrder(TestSearch search, boolean isAscending) {
        var order = isAscending ? YqlOrderBy.SortOrder.ASC : YqlOrderBy.SortOrder.DESC;
        if (!search.getProject().isEmpty()) {
            return List.of(
                    YqlOrderBy.orderBy(
                            new YqlOrderBy.SortKey("id.branch", order),
                            new YqlOrderBy.SortKey("service", order),
                            new YqlOrderBy.SortKey("path", order),
                            new YqlOrderBy.SortKey("id.testId", order),
                            new YqlOrderBy.SortKey("id.suiteId", order),
                            new YqlOrderBy.SortKey("id.toolchain", order)
                    )
            );
        }

        return List.of(
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.branch", order),
                        new YqlOrderBy.SortKey("path", order),
                        new YqlOrderBy.SortKey("id.testId", order),
                        new YqlOrderBy.SortKey("id.suiteId", order),
                        new YqlOrderBy.SortKey("id.toolchain", order)
                )
        );
    }

    /* optimized
    private Pair<String, YqlStatementPart<?>> selectIndexAndOrder(TestSearch search) {
        var order = search.getPaging().isAscending() ? YqlOrderBy.SortOrder.ASC : YqlOrderBy.SortOrder.DESC;
        if (!search.getProject().isEmpty()) {
            return Pair.of(
                    TestStatusEntity.IDX_BRANCH_SERVICE_PATH_TEST_ID,
                    YqlOrderBy.orderBy(
                            new YqlOrderBy.SortKey("id.branch", order),
                            new YqlOrderBy.SortKey("service", order),
                            new YqlOrderBy.SortKey("path", order),
                            new YqlOrderBy.SortKey("id.testId", order),
                            new YqlOrderBy.SortKey("id.suiteId", order),
                            new YqlOrderBy.SortKey("id.toolchain", order)
                    )
            );
        }

        return Pair.of(
                TestStatusEntity.IDX_BRANCH_PATH_TEST_ID,
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.branch", order),
                        new YqlOrderBy.SortKey("path", order),
                        new YqlOrderBy.SortKey("id.testId", order),
                        new YqlOrderBy.SortKey("id.suiteId", order),
                        new YqlOrderBy.SortKey("id.toolchain", order)
                )
        );
    }
     */
}
