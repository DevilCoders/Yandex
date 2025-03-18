package ru.yandex.ci.storage.core.db.model.revision;

import yandex.cloud.repository.kikimr.table.KikimrTable;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class RevisionTable extends KikimrTableCi<RevisionEntity> {
    public RevisionTable(KikimrTable.QueryExecutor executor) {
        super(RevisionEntity.class, executor);
    }
}
