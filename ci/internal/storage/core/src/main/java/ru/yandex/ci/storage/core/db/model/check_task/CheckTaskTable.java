package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.List;
import java.util.stream.Stream;

import lombok.Value;

import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class CheckTaskTable extends KikimrTableCi<CheckTaskEntity> {
    public CheckTaskTable(QueryExecutor executor) {
        super(CheckTaskEntity.class, executor);
    }

    public long countActive(boolean right) {
        return this.count(
                YqlPredicate.and(
                        CheckStatusUtils.getIsActive("status"),
                        YqlPredicate.where("right").eq(right)
                ),
                YqlView.index(CheckTaskEntity.IDX_BY_STATUS_AND_RIGHT)
        );
    }

    public List<CheckTaskEntity> getByIteration(CheckIterationEntity.Id id) {
        return this.find(
                YqlPredicate.where("id.iterationId").eq(id),
                YqlOrderBy.orderBy("id")
        );
    }

    public List<JobNameView> findJobNames(CheckEntity.Id id, CheckIteration.IterationType iterationType) {
        return this.findDistinct(JobNameView.class,
                List.of(
                        YqlPredicate.where("id.iterationId.checkId").eq(id),
                        YqlPredicate.where("id.iterationId.iterationType").eq(iterationType.getNumber()),
                        YqlOrderBy.orderBy("jobName")
                )
        );
    }

    public long countActiveInIteration(CheckIterationEntity.Id id) {
        return this.count(
                YqlPredicate
                        .where("id.iterationId").eq(id)
                        .and(CheckStatusUtils.getIsActive("status"))
        );
    }

    public Stream<CheckTaskEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = CheckTaskEntity.Id.builder()
                .iterationId(new CheckIterationEntity.Id(checkId, Integer.MIN_VALUE, Integer.MIN_VALUE))
                .build();
        var toId = CheckTaskEntity.Id.builder()
                .iterationId(new CheckIterationEntity.Id(checkId, Integer.MAX_VALUE, Integer.MAX_VALUE))
                .build();

        return this.readTableIds(
                ReadTableParams.<CheckTaskEntity.Id>builder()
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }

    @Value
    public static class JobNameView implements Table.View {
        String jobName;
    }
}
