package ru.yandex.ci.common.temporal.monitoring;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import ru.yandex.ci.common.ydb.KikimrTableCi;

public class TemporalFailingWorkflowTable extends KikimrTableCi<TemporalFailingWorkflowEntity> {
    public TemporalFailingWorkflowTable(KikimrTable.QueryExecutor executor) {
        super(TemporalFailingWorkflowEntity.class, executor);
    }
}
