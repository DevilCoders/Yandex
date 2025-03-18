package ru.yandex.ci.core.db.autocheck.table;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.db.autocheck.model.PoolNameByACEntity;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class DistBuildPoolNameByACTable extends KikimrTableCi<PoolNameByACEntity> {
    public DistBuildPoolNameByACTable(QueryExecutor queryExecutor) {
        super(PoolNameByACEntity.class, queryExecutor);
    }

    public Optional<Instant> findLastUpdateTime() {
        var ret = find(PoolNameByACEntity.class,
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1));
        return single(ret).map(rec -> rec.getId().getUpdated());
    }

    public List<PoolNameByACEntity> listPoolsByUpdateTime(Instant updatedTm) {
        return find(YqlPredicate.where("id.updated").eq(updatedTm),
                YqlOrderBy.orderBy("id"));
    }
}
