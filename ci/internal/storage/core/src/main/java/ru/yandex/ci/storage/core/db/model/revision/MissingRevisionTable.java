package ru.yandex.ci.storage.core.db.model.revision;

import yandex.cloud.repository.kikimr.table.KikimrTable;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class MissingRevisionTable extends KikimrTableCi<MissingRevisionEntity> {
    public MissingRevisionTable(KikimrTable.QueryExecutor executor) {
        super(MissingRevisionEntity.class, executor);
    }
}
