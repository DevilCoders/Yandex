package ru.yandex.ci.core.discovery;

import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcRevision;

public class GraphDiscoveryTaskTable extends KikimrTableCi<GraphDiscoveryTask> {

    public GraphDiscoveryTaskTable(QueryExecutor executor) {
        super(GraphDiscoveryTask.class, executor);
    }

    public Optional<GraphDiscoveryTask> findBySandboxTaskId(long sandboxTaskId) {
        return single(
                find(
                        YqlPredicate.where("sandboxTaskId").eq(sandboxTaskId),
                        YqlView.index(GraphDiscoveryTask.IDX_SANDBOX_TASK_ID)
                )
        );
    }

    public List<GraphDiscoveryTask> findByArcRevision(ArcRevision revision) {
        return find(
                YqlPredicate.where("id.commitId").eq(revision.getCommitId())
        );
    }

    public List<GraphDiscoveryTask> findByStatusAndSandboxTaskIdGreaterThen(
            GraphDiscoveryTask.Status status, long sandboxTaskId, int limit
    ) {
        var parts = filter(limit,
                YqlPredicate.where("status").eq(status)
                        .and(YqlPredicate.where("sandboxTaskId").gt(sandboxTaskId)),
                YqlView.index(GraphDiscoveryTask.IDX_STATUS_AND_SANDBOX_TASK_ID),
                YqlOrderBy.orderBy("sandboxTaskId", YqlOrderBy.SortOrder.ASC));
        return find(parts);
    }
}
