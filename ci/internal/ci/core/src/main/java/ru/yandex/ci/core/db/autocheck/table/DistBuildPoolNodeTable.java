package ru.yandex.ci.core.db.autocheck.table;

import java.time.Instant;
import java.util.List;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.db.autocheck.model.PoolNodeEntity;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class DistBuildPoolNodeTable extends KikimrTableCi<PoolNodeEntity> {
    public DistBuildPoolNodeTable(QueryExecutor queryExecutor) {
        super(PoolNodeEntity.class, queryExecutor);
    }

    public List<PoolNodeEntity> listPoolsByUpdateTime(Instant updatedTm) {
        return find(YqlPredicate.where("id.updated").eq(updatedTm),
                YqlOrderBy.orderBy("id"));
    }
}
