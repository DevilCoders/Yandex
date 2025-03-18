package ru.yandex.ci.core.tasklet;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class TaskletMetadataTable extends KikimrTableCi<TaskletMetadata> {

    public TaskletMetadataTable(QueryExecutor executor) {
        super(TaskletMetadata.class, executor);
    }
}
