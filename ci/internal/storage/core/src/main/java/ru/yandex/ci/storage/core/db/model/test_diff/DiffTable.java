package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.List;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;

@Slf4j
public class DiffTable<T extends Entity<T>> extends KikimrTableCi<T> {
    private final Class<T> type;

    public DiffTable(Class<T> type, KikimrTable.QueryExecutor executor) {
        super(type, executor);
        this.type = type;
    }

    public List<T> searchSuites(
            CheckIterationEntity.Id id, SuiteSearchFilters filters, int pageSize
    ) {
        var filter = SearchDiffQueries.getSearchSuitesFilter(id, filters);
        var order = SearchDiffQueries.fillPageFilters(filter, filters.getPageCursor());

        var yql = filter.get().toYql(EntitySchema.of(type));
        log.info("Search suites, filter: {}", yql);
        var orderBy = YqlOrderBy.orderBy(
                new YqlOrderBy.SortKey("id.path", order), new YqlOrderBy.SortKey("id.suiteId", order)
        );
        var limit = YqlLimit.range(0, pageSize);

        if (filters.getTestName().isEmpty() && filters.getSubtestName().isEmpty()) {
            return this.find(filter.get(), orderBy, limit);
        }

        if (filters.getTestName().isEmpty() || filters.getSubtestName().isEmpty()) {
            var entityType = Common.CheckSearchEntityType.CSET_TEST_NAME;
            var value = filters.getLowerTestName();
            if (filters.getTestName().isEmpty()) {
                entityType = Common.CheckSearchEntityType.CSET_SUBTEST_NAME;
                value = filters.getLowerSubtestName();
            }

            return this.executor.execute(
                    DiffsStatement.diffTextSearch(
                            this.type,
                            filter.get(), orderBy, limit, entityType, value
                    ),
                    List.of(filter.get(), orderBy, limit)
            );
        }

        return this.executor.execute(
                DiffsStatement.diffTextSearch(
                        this.type, CheckTextSearchEntity.class,
                        filter.get(), orderBy, limit,
                        filters.getLowerTestName(), filters.getLowerSubtestName()
                ),
                List.of(filter.get(), orderBy, limit)
        );
    }
}
