package ru.yandex.ci.storage.core.db.model.test_launch;

import java.util.Comparator;
import java.util.Map;
import java.util.stream.Collectors;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test_launch.statement.NumberOfLaunchesByStatusStatement;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class TestLaunchTable extends KikimrTableCi<TestLaunchEntity> {
    public TestLaunchTable(KikimrTable.QueryExecutor executor) {
        super(TestLaunchEntity.class, executor);
    }

    public Map<Common.TestStatus, Long> countLaunches(TestStatusEntity.Id statusId, long revision) {
        var result = executeOnView(new NumberOfLaunchesByStatusStatement(statusId, revision), null);
        return result.stream().collect(
                Collectors.toMap(
                        NumberOfLaunchesByStatusStatement.Result::getStatus,
                        NumberOfLaunchesByStatusStatement.Result::getNumber
                )
        );
    }

    public Map<Common.TestStatus, TestLaunchEntity> getLastLaunches(
            TestStatusEntity.Id statusId, long revision
    ) {
        return this.readTable(
                        ReadTableParams.<TestLaunchEntity.Id>builder()
                                .fromKey(new TestLaunchEntity.Id(statusId, revision, null))
                                .toKey(new TestLaunchEntity.Id(statusId, revision + 1, null))
                                .fromInclusive(true)
                                .toInclusive(false)
                                .ordered()
                                .build()
                ).sorted(Comparator.comparing(TestLaunchEntity::getProcessedAt))
                .collect(
                        Collectors.toMap(
                                TestLaunchEntity::getStatus, x -> x, (a, b) -> b
                        )
                );
    }
}
