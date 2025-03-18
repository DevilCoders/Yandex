package ru.yandex.ci.storage.core.db.model.task_result;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import com.google.common.primitives.UnsignedLongs;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@SuppressWarnings("UnstableApiUsage")
public class TestResultTable extends KikimrTableCi<TestResultEntity> {

    public TestResultTable(QueryExecutor executor) {
        super(TestResultEntity.class, executor);
    }

    public List<TestResultEntity> find(
            CheckEntity.Id checkId,
            CheckIteration.IterationType iterationType,
            long suiteId,
            String toolchain,
            Long testId
    ) {
        var predicate = YqlPredicate
                .where("id.checkId").eq(checkId)
                .and("id.iterationType").eq(iterationType.getNumber())
                .and("id.suiteId").eq(suiteId)
                .and("id.testId").eq(testId);

        if (toolchain.equals(TestEntity.ALL_TOOLCHAINS)) {
            predicate = predicate.and("id.toolchain").neq(TestEntity.ALL_TOOLCHAINS);
        } else {
            predicate = predicate.and("id.toolchain").eq(toolchain);
        }

        return this.find(predicate);
    }

    @Nullable
    public TestResultEntity find(
            CheckIterationEntity.Id iterationId,
            TestEntity.Id testId,
            String taskId,
            int partition,
            int retryNumber
    ) {
        var predicate = YqlPredicate
                .where("id.checkId").eq(iterationId.getCheckId())
                .and("id.iterationType").eq(iterationId.getIterationTypeNumber())
                .and("id.iterationNumber").eq(iterationId.getNumber())
                .and("id.suiteId").eq(testId.getSuiteId())
                .and("id.toolchain").eq(testId.getToolchain())
                .and("id.testId").eq(testId.getId())
                .and("id.taskId").eq(taskId)
                .and("id.partition").eq(partition)
                .and("id.retryNumber").eq(retryNumber);

        var result = this.find(predicate);
        if (result.isEmpty()) {
            return null;
        }

        return result.get(0);
    }

    public List<TestResultEntity> findSuites(
            CheckIterationEntity.Id iterationId,
            Collection<Long> suiteIds
    ) {
        var partialIds = suiteIds.stream().map(
                id -> new TestResultEntity.Id(
                        iterationId.getCheckId(),
                        iterationId.getIterationType().getNumber(),
                        id,
                        id,
                        null,
                        null,
                        null,
                        null,
                        null
                )
        ).distinct().collect(Collectors.toList());

        return Lists.partition(partialIds, 64).stream()
                .flatMap(batch -> this.find(new HashSet<>(batch)).stream())
                .filter(x -> x.getId().getIterationNumber() == iterationId.getNumber())
                .toList();
    }

    public Stream<TestResultEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = TestResultEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MIN_VALUE)
                .build();
        var toId = TestResultEntity.Id.builder()
                .checkId(checkId)
                .iterationType(Integer.MAX_VALUE)
                .build();

        return this.readTableIds(
                ReadTableParams.<TestResultEntity.Id>builder()
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }

    public Stream<TestResultEntity> streamCircuit(
            CheckEntity.Id checkId, CheckIteration.IterationType iterationType
    ) {
        var fromId = TestResultEntity.Id.builder()
                .checkId(checkId)
                .iterationType(iterationType.getNumber())
                .suiteId(0L)
                .build();
        var toId = TestResultEntity.Id.builder()
                .checkId(checkId)
                .iterationType(iterationType.getNumber())
                .suiteId(UnsignedLongs.MAX_VALUE)
                .build();

        return this.readTable(
                ReadTableParams.<TestResultEntity.Id>builder()
                        .useNewSpliterator(true)
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .build()
        );
    }
}
