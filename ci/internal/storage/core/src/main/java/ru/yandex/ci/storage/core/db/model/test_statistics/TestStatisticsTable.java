package ru.yandex.ci.storage.core.db.model.test_statistics;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;


public class TestStatisticsTable extends KikimrTableCi<TestStatisticsEntity> {

    public TestStatisticsTable(QueryExecutor executor) {
        super(TestStatisticsEntity.class, executor);
    }
}
