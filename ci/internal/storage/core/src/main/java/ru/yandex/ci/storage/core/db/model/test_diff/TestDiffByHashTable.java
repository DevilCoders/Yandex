package ru.yandex.ci.storage.core.db.model.test_diff;

import java.time.Duration;
import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.primitives.UnsignedLong;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.exception.RepositoryException;
import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.util.ObjectStore;
import ru.yandex.ci.util.Retryable;

@Slf4j
public class TestDiffByHashTable extends KikimrTableCi<TestDiffByHashEntity> {
    public TestDiffByHashTable(QueryExecutor executor) {
        super(TestDiffByHashEntity.class, executor);
    }

    @SuppressWarnings("unchecked")
    public List<TestDiffByHashEntity> getChunk(ChunkAggregateEntity.Id aggregateId) {
        return getChunksImpl(aggregateId, this::readTable);
    }

    public List<TestDiffByHashEntity.Id> getChunkIds(ChunkAggregateEntity.Id aggregateId) {
        return getChunksImpl(aggregateId, this::readTableIds);
    }

    private <T> List<T> getChunksImpl(
            ChunkAggregateEntity.Id aggregateId,
            Function<ReadTableParams<TestDiffByHashEntity.Id>, Stream<T>> loader
    ) {
        return Retryable.retryUntilInterruptedOrSucceeded(
                () -> loader.apply(getChunkReadTableParams(aggregateId)).collect(Collectors.toList()),
                (e) -> {
                    if (e instanceof RepositoryException) {
                        log.warn("Failed to load aggregate diffs {}, message: {}", aggregateId, e.getMessage());
                    } else {
                        log.error("Failed to load aggregate diffs {}, message: {}", aggregateId, e.getMessage());
                    }
                }
        );
    }

    private ReadTableParams<TestDiffByHashEntity.Id> getChunkReadTableParams(ChunkAggregateEntity.Id aggregateId) {
        return ReadTableParams.<TestDiffByHashEntity.Id>builder()
                .fromKey(TestDiffByHashEntity.Id.of(aggregateId, null))
                .toKey(
                        TestDiffByHashEntity.Id.of(
                                aggregateId,
                                new TestEntity.Id(
                                        UnsignedLong.MAX_VALUE.longValue(),
                                        String.valueOf(Character.MAX_VALUE),
                                        UnsignedLong.MAX_VALUE.longValue()
                                )
                        )
                )
                .toInclusive(true)
                .fromInclusive(true)
                .timeout(Duration.ofMinutes(5))
                .ordered()
                .build();
    }

    public List<TestDiffByHashEntity> getSuite(ChunkAggregateEntity.Id aggregateId, long suiteId, String toolchain) {
        return this.readTable(
                ReadTableParams.<TestDiffByHashEntity.Id>builder()
                        .fromKey(
                                TestDiffByHashEntity.Id.of(
                                        aggregateId,
                                        new TestEntity.Id(suiteId, toolchain, UnsignedLong.ZERO.longValue())
                                )
                        )
                        .toKey(
                                TestDiffByHashEntity.Id.of(
                                        aggregateId,
                                        new TestEntity.Id(suiteId, toolchain, UnsignedLong.MAX_VALUE.longValue())
                                )
                        )
                        .toInclusive(true)
                        .fromInclusive(true)
                        .timeout(Duration.ofMinutes(1))
                        .ordered()
                        .build()
        ).collect(Collectors.toList());
    }

    public List<TestDiffByHashEntity> listSuite(
            int aggregateHash, TestDiffEntity.Id diffId, SuiteSearchFilters filters, int skip, int take
    ) {
        final var filter = new ObjectStore<>(
                YqlPredicate
                        .where("id.aggregateIdHash").eq(aggregateHash)
                        .and("id.aggregateId.iterationId.checkId").eq(diffId.getCheckId())
                        .and("id.aggregateId.iterationId.iterationType").eq(diffId.getIterationType())
                        .and("id.testId.toolchain").eq(diffId.getToolchain())
                        .and("id.testId.suiteId").eq(diffId.getSuiteId())
                        .and("id.testId.id").neq(diffId.getSuiteId())
        );

        if (diffId.getIterationNumber() == 0) {
            filter.set(filter.get().and(YqlPredicate.where("isLast").eq(true)));
        } else {
            filter.set(
                    filter.get().and(YqlPredicate.where("id.aggregateId.iterationId.number")
                            .eq(diffId.getIterationNumber()))
            );
        }

        SearchDiffQueries.fillCommonFilters(filters, filter);

        if (!filters.getTestName().isEmpty()) {
            filter.set(filter.get().and(YqlPredicate.where("name").eq(filters.getTestName())));
        }

        if (!filters.getSubtestName().isEmpty()) {
            filter.set(filter.get().and(YqlPredicate.where("subtestName").eq(filters.getSubtestName())));
        }

        return this.find(
                filter.get(),
                YqlOrderBy.orderBy(
                        new YqlOrderBy.SortKey("path", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("name", YqlOrderBy.SortOrder.ASC),
                        new YqlOrderBy.SortKey("subtestName", YqlOrderBy.SortOrder.ASC)
                ),
                YqlLimit.range(skip, take)
        );
    }
}
