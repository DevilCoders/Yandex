package ru.yandex.ci.core.db.table;

import java.util.List;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class AutoReleaseQueueTable extends KikimrTableCi<AutoReleaseQueueItem> {

    public AutoReleaseQueueTable(QueryExecutor executor) {
        super(AutoReleaseQueueItem.class, executor);
    }

    public List<AutoReleaseQueueItem> findByState(AutoReleaseQueueItem.State state) {
        var parts = filter(
                YqlPredicate.where("state").eq(state),
                YqlView.index(AutoReleaseQueueItem.IDX_BY_STATE),
                YqlOrderBy.orderBy("id"));
        return find(parts);
    }

    public List<AutoReleaseQueueItem> findByProcessIdAndState(
            CiProcessId processId,
            AutoReleaseQueueItem.State state) {
        var parts = filter(
                YqlPredicate.where("id.processId").eq(processId.asString())
                        .and(YqlPredicate.where("state").eq(state)),
                YqlView.index(AutoReleaseQueueItem.IDX_BY_PROCESS_ID_AND_STATE),
                YqlOrderBy.orderBy("id"));
        return find(parts);
    }

    public List<AutoReleaseQueueItem> findByProcessId(CiProcessId processId) {
        return find(filter(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlView.index(AutoReleaseQueueItem.IDX_BY_PROCESS_ID_AND_STATE),
                YqlOrderBy.orderBy("id"))
        );
    }
}
