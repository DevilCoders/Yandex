package ru.yandex.ci.core.db.table;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.db.model.TrackerFlow;

public class TrackerFlowTable extends KikimrTableCi<TrackerFlow> {
    public TrackerFlowTable(KikimrTable.QueryExecutor executor) {
        super(TrackerFlow.class, executor);
    }
}
