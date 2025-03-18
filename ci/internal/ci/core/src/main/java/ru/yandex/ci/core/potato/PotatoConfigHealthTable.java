package ru.yandex.ci.core.potato;

import java.time.Instant;
import java.util.List;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class PotatoConfigHealthTable extends KikimrTableCi<ConfigHealth> {
    public PotatoConfigHealthTable(QueryExecutor queryExecutor) {
        super(ConfigHealth.class, queryExecutor);
    }

    public List<ConfigHealth> findSeenAfter(Instant instant) {
        return find(YqlPredicate.where("lastSeen").gt(instant));
    }
}
