package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.List;
import java.util.Set;
import java.util.stream.Stream;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class LargeTaskTable extends KikimrTableCi<LargeTaskEntity> {

    public LargeTaskTable(KikimrTable.QueryExecutor executor) {
        super(LargeTaskEntity.class, executor);
    }

    public List<LargeTaskEntity> findAllLargeTasks(
            CheckEntity.Id checkId,
            CheckIteration.IterationType iterationType,
            Set<Common.CheckTaskType> checkTypes) {
        return find(
                YqlPredicate.where("id.iterationId.checkId").eq(checkId),
                YqlPredicate.where("id.iterationId.iterationType").in(iterationType.getNumber()),
                YqlPredicate.where("id.checkTaskType").in(checkTypes)
        );
    }

    public Stream<LargeTaskEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = new CheckIterationEntity.Id(checkId, Integer.MIN_VALUE, Integer.MIN_VALUE);
        var toId = new CheckIterationEntity.Id(checkId, Integer.MAX_VALUE, Integer.MAX_VALUE);

        return this.readTableIds(
                ReadTableParams.<LargeTaskEntity.Id>builder()
                        .fromKey(new LargeTaskEntity.Id(fromId, Common.CheckTaskType.UNRECOGNIZED, 0))
                        .toKey(new LargeTaskEntity.Id(toId, Common.CheckTaskType.UNRECOGNIZED, 0))
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
