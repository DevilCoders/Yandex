package ru.yandex.ci.core.job;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;

public class JobInstanceTable extends KikimrTableCi<JobInstance> {

    public JobInstanceTable(QueryExecutor executor) {
        super(JobInstance.class, executor);
    }

    public List<JobInstance> findRunning() {
        var running = Stream.of(JobStatus.values())
                .filter(f -> !f.isFinished())
                .collect(Collectors.toList());
        return find(YqlPredicateCi.in("status", running));
    }
}
