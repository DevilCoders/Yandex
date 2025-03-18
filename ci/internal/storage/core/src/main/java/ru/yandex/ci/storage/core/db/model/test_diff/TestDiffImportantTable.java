package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.List;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@Slf4j
public class TestDiffImportantTable extends DiffTable<TestDiffImportantEntity> {
    public TestDiffImportantTable(QueryExecutor executor) {
        super(TestDiffImportantEntity.class, executor);
    }

    public List<TestDiffImportantEntity> searchSuitesNotAllToolchains(
            CheckIterationEntity.Id id, SuiteSearchFilters filters, int pageSize
    ) {
        var filter = SearchDiffQueries.getSearchSuitesFilterNotAllToolchain(id, filters);
        var order = SearchDiffQueries.fillPageFilters(filter, filters.getPageCursor());

        log.info("Search suites, filter: {}", filter.get());

        return this.find(
                filter.get(),
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("id.path", order), new YqlOrderBy.SortKey("id.suiteId", order)
                ),
                YqlLimit.range(0, pageSize)
        );
    }
}
