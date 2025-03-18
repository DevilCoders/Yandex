package ru.yandex.ci.storage.core.db.model.test_revision;

import java.util.List;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class TestImportantRevisionTable extends KikimrTableCi<TestImportantRevisionEntity> {
    public TestImportantRevisionTable(KikimrTable.QueryExecutor executor) {
        super(TestImportantRevisionEntity.class, executor);
    }

    public List<Long> getHistoryRevisions(
            TestStatusEntity.Id statusId, HistoryFilters filters, HistoryPaging historyPaging, int pageSize
    ) {
        var predicate = YqlPredicate
                .where("id.testId").eq(statusId.getTestId())
                .and("id.suiteId").eq(statusId.getSuiteId())
                .and("id.branch").eq(statusId.getBranch())
                .and("changed").eq(true);

        if (!statusId.getToolchain().isEmpty() && !statusId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            predicate = predicate.and("id.toolchain").eq(statusId.getToolchain());
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
            predicate = predicate.and(YqlPredicate.where("previousStatus").eq(status).or("status").eq(status));
        }

        var order = YqlOrderBy.SortOrder.DESC;

        if (historyPaging.getFromRevision() > 0) {
            predicate = predicate.and("id.revision").lte(historyPaging.getFromRevision());
        } else if (historyPaging.getToRevision() > 0) {
            order = YqlOrderBy.SortOrder.ASC;
            predicate = predicate.and("id.revision").gt(historyPaging.getToRevision());
        }

        var orderBy = YqlOrderBy.orderBy(
                new YqlOrderBy.SortKey("id.testId", order),
                new YqlOrderBy.SortKey("id.suiteId", order),
                new YqlOrderBy.SortKey("id.branch", order),
                new YqlOrderBy.SortKey("id.revision", order)
        );

        var limit = YqlLimit.range(0, pageSize);
        return this.findDistinct(
                TestImportantRevisionEntity.RevisionView.class,
                List.of(predicate, orderBy, limit)
        ).stream().map(x -> x.getId().getRevision()).collect(Collectors.toList());
    }
}
