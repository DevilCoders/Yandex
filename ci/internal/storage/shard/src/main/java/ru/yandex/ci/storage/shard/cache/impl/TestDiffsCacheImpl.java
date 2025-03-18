package ru.yandex.ci.storage.shard.cache.impl;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.impl.EntityGroupCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.GroupingEntityCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.shard.cache.TestDiffsCache;

@Slf4j
public class TestDiffsCacheImpl extends GroupingEntityCacheImpl<ChunkAggregateEntity.Id, TestDiffByHashEntity.Id,
        TestDiffByHashEntity> implements TestDiffsCache, TestDiffsCache.WithModificationSupport {
    private final CiStorageDb db;

    public TestDiffsCacheImpl(CiStorageDb db, int capacity, MeterRegistry meterRegistry) {
        super(
                CacheBuilder.newBuilder()
                        .maximumSize(capacity)
                        .expireAfterAccess(Duration.ofHours(1))
                        .recordStats()
                        .build(),
                meterRegistry,
                "chunk-result-diffs"
        );
        this.db = db;

        GuavaCacheMetrics.monitor(meterRegistry, this.cache, "chunk-result-diffs");
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, cache, maxNumberOfWrites);
    }

    @Override
    protected ChunkAggregateEntity.Id getAggregateId(TestDiffByHashEntity.Id id) {
        return id.getAggregateId();
    }

    @Override
    protected ChunkCache load(ChunkAggregateEntity.Id aggregateId) {
        return new ChunkCache(this.db.readOnly().run(() -> this.db.testDiffsByHash().getChunk(aggregateId)));
    }

    @Override
    protected EntityGroupCacheImpl<TestDiffByHashEntity.Id, TestDiffByHashEntity> loadForEmpty(
            ChunkAggregateEntity.Id id
    ) {
        return new ChunkCache(new ArrayList<>());
    }

    @Override
    public Collection<TestDiffByHashEntity> getByParentId(TestDiffByHashEntity.Id id) {
        return this.get(id.getAggregateId()).getAll().stream()
                .filter(diff -> diff.getId().getTestId().getSuiteId() == id.getTestId().getSuiteId())
                .filter(diff -> diff.getId().getTestId().getToolchain().equals(id.getTestId().getToolchain()))
                .collect(Collectors.toList());
    }

    @Override
    public TestDiffByHashEntity getOrCreate(TestDiffByHashEntity.Id id, TestResult result) {
        return get(id)
                .orElseGet(
                        () -> {
                            var resultType = id.getTestId().isSuite()
                                    ? ResultTypeUtils.toSuiteType(result.getResultType())
                                    : result.getResultType();

                            var strongModeAYaml = id.getTestId().isSuite()
                                    ? result.getStrongModeAYaml()
                                    : "";

                            return TestDiffByHashEntity.builder()
                                    .id(id)
                                    .resultType(resultType)
                                    .path(result.getPath())
                                    .statistics(TestDiffStatistics.EMPTY)
                                    // last diff always stored in first iteration or in meta iteration.
                                    .isLast(id.getAggregateId().getIterationId().getNumber() <= 1)
                                    .isStrongMode(result.isStrongMode())
                                    .strongModeAYaml(strongModeAYaml)
                                    .processedBy(result.getProcessedBy())
                                    .autocheckChunkId(result.getAutocheckChunkId())
                                    .isOwner(result.isOwner())
                                    .owners(result.getOwners())
                                    .oldTestId(result.getOldTestId())
                                    .oldSuiteId(result.getOldSuiteId())
                                    .build();
                        }
                );
    }

    @Override
    public Optional<TestDiffByHashEntity> findInPreviousIterations(
            TestDiffByHashEntity.Id diffId,
            int iterationNumber
    ) {
        for (int i = iterationNumber - 1; i > 0; --i) {
            var diff = get(TestDiffByHashEntity.Id.of(diffId.getAggregateId().toIterationId(i), diffId.getTestId()));
            if (diff.isPresent()) {
                return diff;
            }
        }

        return Optional.empty();
    }

    @Override
    protected KikimrTableCi<TestDiffByHashEntity> getTable() {
        return db.testDiffsByHash();
    }

    private static class ChunkCache extends EntityGroupCacheImpl<TestDiffByHashEntity.Id, TestDiffByHashEntity> {
        ChunkCache(List<TestDiffByHashEntity> diffs) {
            super(diffs);
        }
    }

    public static class Modifiable extends GroupingEntityCacheImpl.Modifiable<
            ChunkAggregateEntity.Id, TestDiffByHashEntity.Id, TestDiffByHashEntity
            > implements TestDiffsCache.WithCommitSupport {
        public Modifiable(
                TestDiffsCacheImpl baseImpl,
                Cache<ChunkAggregateEntity.Id, EntityGroupCacheImpl<TestDiffByHashEntity.Id,
                        TestDiffByHashEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public Collection<TestDiffByHashEntity> getByParentId(TestDiffByHashEntity.Id id) {
            var resultsMap = ((TestDiffsCacheImpl) this.baseImpl).getByParentId(id).stream()
                    .collect(Collectors.toMap(TestDiffByHashEntity::getId, Function.identity()));

            this.writeBuffer.values().stream()
                    .filter(diff -> diff.getId().getTestId().getSuiteId() == id.getTestId().getSuiteId())
                    .filter(diff -> diff.getId().getTestId().getToolchain().equals(id.getTestId().getToolchain()))
                    .forEach(result -> resultsMap.put(result.getId(), result));

            return resultsMap.values();
        }

        @Override
        public TestDiffByHashEntity getOrCreate(TestDiffByHashEntity.Id id, TestResult taskResult) {
            var fromBuffer = this.writeBuffer.get(id);
            if (fromBuffer != null) {
                return fromBuffer;
            }
            return ((TestDiffsCacheImpl) this.baseImpl).getOrCreate(id, taskResult);
        }

        @Override
        public Optional<TestDiffByHashEntity> findInPreviousIterations(
                TestDiffByHashEntity.Id diffId,
                int iterationNumber
        ) {
            for (int i = iterationNumber - 1; i > 0; --i) {
                var diff = get(
                        TestDiffByHashEntity.Id.of(diffId.getAggregateId().toIterationId(i), diffId.getTestId())
                );
                if (diff.isPresent()) {
                    return diff;
                }
            }

            return Optional.empty();
        }

        @Override
        public void invalidate(Set<ChunkEntity.Id> chunks) {
            var diffsToRemove = this.cache.asMap().keySet().stream()
                    .filter(x -> chunks.contains(x.getChunkId()))
                    .collect(Collectors.toSet());

            log.info("Invalidating {} diffs", diffsToRemove.size());

            this.cache.invalidateAll(diffsToRemove);
        }
    }
}
