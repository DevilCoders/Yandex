package ru.yandex.ci.storage.core.db.model.groups;

import yandex.cloud.repository.kikimr.table.KikimrTable;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class GroupTable extends KikimrTableCi<GroupEntity> {
    public GroupTable(KikimrTable.QueryExecutor executor) {
        super(GroupEntity.class, executor);
    }
}
