package ru.yandex.ci.storage.core.db.model.check_task_statistics;

import java.util.List;
import java.util.stream.Stream;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

public class CheckTaskStatisticsTable extends KikimrTableCi<CheckTaskStatisticsEntity> {

    public CheckTaskStatisticsTable(QueryExecutor executor) {
        super(CheckTaskStatisticsEntity.class, executor);
    }

    public List<CheckTaskStatisticsEntity> getByIteration(CheckIterationEntity.Id id) {
        return this.find(YqlPredicate.where("id.taskId.iterationId").eq(id));
    }

    public Stream<CheckTaskStatisticsEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = CheckTaskEntity.Id.builder()
                .iterationId(new CheckIterationEntity.Id(checkId, Integer.MIN_VALUE, Integer.MIN_VALUE))
                .build();
        var toId = CheckTaskEntity.Id.builder()
                .iterationId(new CheckIterationEntity.Id(checkId, Integer.MAX_VALUE, Integer.MAX_VALUE))
                .build();

        return this.readTableIds(
                ReadTableParams.<CheckTaskStatisticsEntity.Id>builder()
                        .fromKey(new CheckTaskStatisticsEntity.Id(fromId, Integer.MIN_VALUE, null))
                        .toKey(new CheckTaskStatisticsEntity.Id(toId, Integer.MAX_VALUE, null))
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
