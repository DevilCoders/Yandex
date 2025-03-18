package ru.yandex.ci.observer.core.db.model.traces;

import java.util.List;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;

public class CheckTaskPartitionTracesTable extends KikimrTableCi<CheckTaskPartitionTraceEntity> {

    public CheckTaskPartitionTracesTable(QueryExecutor executor) {
        super(CheckTaskPartitionTraceEntity.class, executor);
    }

    public List<CheckTaskPartitionTraceEntity> findByCheckTaskPartition(CheckTaskEntity.Id id, int partition) {
        return find(List.of(
                YqlPredicate
                        .where("id.taskId").eq(id)
                        .and("id.partition").eq(partition),
                YqlOrderBy.orderBy("id")
        ));
    }

    public List<CheckTaskPartitionTraceEntity> findByCheckTask(CheckTaskEntity.Id id) {
        return find(List.of(
                YqlPredicate.where("id.taskId").eq(id),
                YqlOrderBy.orderBy("id")
        ));
    }
}
