package ru.yandex.ci.observer.core.db.model.sla_statistics;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.CheckOuterClass;

public class SlaStatisticsTable extends KikimrTableCi<SlaStatisticsEntity> {
    public SlaStatisticsTable(QueryExecutor executor) {
        super(SlaStatisticsEntity.class, executor);
    }

    public List<SlaStatisticsEntity> findRange(List<SlaStatisticsEntity.Id> ids) {
        List<SlaStatisticsEntity> result = new ArrayList<>();
        for (var batch : Lists.partition(ids, YdbUtils.RESULT_ROW_LIMIT)) {
            result.addAll(find(Set.copyOf(batch)));
        }

        return result;
    }

    public List<SlaStatisticsEntity> findRange(
            Instant from, Instant to,
            CheckOuterClass.CheckType checkType,
            IterationTypeGroup iterationTypeGroup,
            IterationCompleteGroup status,
            int windowDays,
            @Nullable String advisedPool,
            String authors,
            @Nullable Long totalNumberOfNodes
    ) {
        var parts = YqlPredicate.where("id.checkType").eq(checkType)
                .and("id.iterationTypeGroup").eq(iterationTypeGroup)
                .and("id.status").eq(status)
                .and("id.windowDays").eq(windowDays)
                .and("id.day").gte(from).and("id.day").lte(to)
                .and(addFilter(advisedPool, "advisedPool"))
                .and(YqlPredicate.where("authors").eq(authors))
                .and(addFilter(totalNumberOfNodes, "totalNumberOfNodes"));

        return find(List.of(parts, YqlOrderBy.orderBy("id")));
    }

    public void saveIfAbsent(SlaStatisticsEntity entity) {
        var id = entity.getId();
        var existing = find(List.of(
                YqlPredicate.where("id.checkType").eq(id.getCheckType())
                        .and("id.iterationTypeGroup").eq(id.getIterationTypeGroup())
                        .and("id.status").eq(id.getStatus())
                        .and("id.windowDays").eq(id.getWindowDays())
                        .and("id.day").eq(id.getDay())
                        .and(addFilter(entity.getAdvisedPool(), "advisedPool"))
                        .and(YqlPredicate.where("authors").eq(entity.getAuthors()))
                        .and(addFilter(entity.getTotalNumberOfNodes(), "totalNumberOfNodes"))
        )).stream().findFirst();

        if (existing.isEmpty()) {
            save(entity);
        }
    }

    private <T> YqlPredicate addFilter(@Nullable T field, String fieldName) {
        if (field == null) {
            return YqlPredicate.where(fieldName).isNull();
        } else {
            return YqlPredicate.where(fieldName).eq(field);
        }
    }
}
