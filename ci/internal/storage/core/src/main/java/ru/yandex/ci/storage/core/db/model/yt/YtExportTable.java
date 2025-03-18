package ru.yandex.ci.storage.core.db.model.yt;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class YtExportTable extends KikimrTableCi<YtExportEntity> {

    public YtExportTable(QueryExecutor executor) {
        super(YtExportEntity.class, executor);
    }
}
