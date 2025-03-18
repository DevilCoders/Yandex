package ru.yandex.ci.storage.core.db.model.test_diff.index;

import java.util.stream.Stream;

import com.google.common.primitives.UnsignedLongs;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@SuppressWarnings("UnstableApiUsage")
public class TestDiffsBySuiteTable extends KikimrTableCi<TestDiffBySuiteEntity> {
    public TestDiffsBySuiteTable(QueryExecutor executor) {
        super(TestDiffBySuiteEntity.class, executor);
    }

    public TestDiffBySuiteEntity findSuite(CheckIterationEntity.Id iterationId, long suiteId) {
        return single(this.find(
                YqlPredicate.where("id.checkId").eq(iterationId.getCheckId().getId()),
                YqlPredicate.where("id.iterationType").eq(iterationId.getIterationTypeNumber()),
                YqlPredicate.where("id.iterationNumber").eq(iterationId.getNumber()),
                YqlPredicate.where("id.toolchain").eq(TestEntity.ALL_TOOLCHAINS),
                YqlPredicate.where("id.suiteId").eq(suiteId),
                YqlPredicate.where("id.testId").eq(suiteId),
                YqlLimit.top(1))
        ).orElseThrow(() -> GrpcUtils.notFoundException(iterationId + ":" + UnsignedLongs.toString(suiteId)));
    }

    public Stream<TestDiffBySuiteEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = TestDiffBySuiteEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MIN_VALUE)
                .build();
        var toId = TestDiffBySuiteEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MAX_VALUE)
                .build();

        return this.readTableIds(
                ReadTableParams.<TestDiffBySuiteEntity.Id>builder()
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
