package ru.yandex.ci.observer.core.db.model.check_tasks;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

import static ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity.IDX_BY_RIGHT_REVISION_TIMESTAMP_CHECK_TYPE_STATUS_ITER_TYPE_JOB_NAME;

public class CheckTasksTable extends KikimrTableCi<CheckTaskEntity> {
    private static final int MAX_PAGE_SIZE = 999;

    public CheckTasksTable(QueryExecutor executor) {
        super(CheckTaskEntity.class, executor);
    }

    public List<CheckTaskEntity> getByIteration(CheckIterationEntity.Id id) {
        var request = List.of(
                YqlPredicate.where("id.iterationId").eq(id),
                YqlOrderBy.orderBy("id"),
                YqlLimit.top(MAX_PAGE_SIZE)
        );

        List<CheckTaskEntity> page = this.find(request);
        List<CheckTaskEntity> result = new ArrayList<>(page);

        while (page.size() >= MAX_PAGE_SIZE) {
            page = find(
                    request.get(0), request.get(1),
                    YqlLimit.range(result.size(), result.size() + MAX_PAGE_SIZE)
            );
            result.addAll(page);
        }

        return result;
    }

    public List<MinRightRevisionTimestamp> getMinRightRevisionTimestampForRunningTasksGroupedByJobName(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull CheckOuterClass.CheckType checkType,
            @Nonnull List<IterationType> iterationTypes
    ) {
        return groupBy(
                MinRightRevisionTimestamp.class,
                List.of(
                        "jobName",
                        "min(`rightRevisionTimestamp`) as minRightRevisionTimestamp",
                        "min_by(id_iterationId_checkId, `rightRevisionTimestamp`) as checkId",
                        "min_by(id_iterationId_iterType, `rightRevisionTimestamp`) as iterType",
                        "min_by(id_iterationId_number, `rightRevisionTimestamp`) as number"
                ),
                List.of("jobName"),
                List.of(
                        YqlPredicate.where("rightRevisionTimestamp").gte(from).and("rightRevisionTimestamp").lte(to)
                                .and(YqlPredicate.where("checkType").eq(checkType))
                                .and(CheckStatusUtils.getRunning("status"))
                                .and(YqlPredicateCi.in("id.iterationId.iterType", iterationTypes)),
                        YqlView.index(IDX_BY_RIGHT_REVISION_TIMESTAMP_CHECK_TYPE_STATUS_ITER_TYPE_JOB_NAME)
                )
        );
    }

    @Value
    @SuppressWarnings("ReferenceEquality")
    public static class MinRightRevisionTimestamp implements View {
        @Nonnull
        String jobName;

        @Column(name = "minRightRevisionTimestamp", dbType = DbType.TIMESTAMP)
        Instant rightRevisionTimestamp;

        @Column
        long checkId;

        @Column
        IterationType iterType;

        @Column
        int number;

        @Column(dbType = DbType.UINT64)
        long count;

        public CheckIterationEntity.Id getIterationId() {
            return new CheckIterationEntity.Id(CheckEntity.Id.of(checkId), iterType, number);
        }
    }

}
