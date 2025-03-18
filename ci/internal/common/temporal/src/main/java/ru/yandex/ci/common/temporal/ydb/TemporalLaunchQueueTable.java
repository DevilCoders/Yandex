package ru.yandex.ci.common.temporal.ydb;

import java.time.Duration;
import java.time.Instant;
import java.util.List;

import yandex.cloud.repository.kikimr.table.KikimrTable;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;

public class TemporalLaunchQueueTable extends KikimrTableCi<TemporalLaunchQueueEntity> {

    public TemporalLaunchQueueTable(KikimrTable.QueryExecutor executor) {
        super(TemporalLaunchQueueEntity.class, executor);
    }

    public List<TemporalLaunchQueueEntity> getLostWorkflows(Duration lostTimeout, int limit) {
        long fromSeconds = Instant.now().minus(lostTimeout).getEpochSecond();
        return find(filter(limit, YqlPredicate.where("enqueueTimeSeconds").lt(fromSeconds)));
    }
}
