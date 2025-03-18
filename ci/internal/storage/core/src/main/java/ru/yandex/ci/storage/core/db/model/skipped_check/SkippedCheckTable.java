package ru.yandex.ci.storage.core.db.model.skipped_check;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class SkippedCheckTable extends KikimrTableCi<SkippedCheckEntity> {
    public SkippedCheckTable(QueryExecutor executor) {
        super(SkippedCheckEntity.class, executor);
    }
}
