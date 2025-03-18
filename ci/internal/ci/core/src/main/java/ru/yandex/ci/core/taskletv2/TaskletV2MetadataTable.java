package ru.yandex.ci.core.taskletv2;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import ru.yandex.ci.common.ydb.KikimrTableCi;

public class TaskletV2MetadataTable extends KikimrTableCi<TaskletV2Metadata> {

    public TaskletV2MetadataTable(QueryExecutor executor) {
        super(TaskletV2Metadata.class, executor);
    }
}
