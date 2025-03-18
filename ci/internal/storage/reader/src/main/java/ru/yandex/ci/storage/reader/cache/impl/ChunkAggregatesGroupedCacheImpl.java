package ru.yandex.ci.storage.reader.cache.impl;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.impl.EntityGroupCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.GroupingEntityCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.reader.cache.ChunkAggregatesGroupedCache;

public class ChunkAggregatesGroupedCacheImpl extends GroupingEntityCacheImpl<CheckIterationEntity.Id,
        ChunkAggregateEntity.Id, ChunkAggregateEntity>
        implements ChunkAggregatesGroupedCache, ChunkAggregatesGroupedCache.WithModificationSupport {

    private final CiStorageDb db;

    public ChunkAggregatesGroupedCacheImpl(CiStorageDb db, int capacity, MeterRegistry meterRegistry) {
        super(
                CacheBuilder.newBuilder()
                        .maximumSize(capacity)
                        .expireAfterAccess(Duration.ofHours(1))
                        .recordStats()
                        .build(),
                meterRegistry,
                "iteration-chunk-aggregates"
        );
        this.db = db;

        GuavaCacheMetrics.monitor(meterRegistry, this.cache, "iteration-chunk-aggregates");
    }

    @Override
    public ChunkAggregatesGroupedCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, cache, maxNumberOfWrites);
    }

    @Override
    protected CheckIterationEntity.Id getAggregateId(ChunkAggregateEntity.Id id) {
        return id.getIterationId();
    }

    @Override
    protected ChunkCache load(CheckIterationEntity.Id iterationId) {
        return new ChunkCache(this.db.readOnly().run(() -> this.db.chunkAggregates().findByIterationId(iterationId)));
    }

    @Override
    protected EntityGroupCacheImpl<ChunkAggregateEntity.Id, ChunkAggregateEntity> loadForEmpty(
            CheckIterationEntity.Id id
    ) {
        return new ChunkCache(new ArrayList<>());
    }

    @Override
    public Collection<ChunkAggregateEntity> getByIterationId(CheckIterationEntity.Id iterationId) {
        return this.get(iterationId).getAll();
    }

    @Override
    public Set<ChunkEntity.Id> getNotCompletedChunkIds(CheckIterationEntity.Id iterationId) {
        return this.getByIterationId(iterationId).stream()
                .filter(x -> !x.isFinished())
                .map(x -> x.getId().getChunkId())
                .collect(Collectors.toSet());
    }

    @Override
    protected KikimrTableCi<ChunkAggregateEntity> getTable() {
        return db.chunkAggregates();
    }

    private static class ChunkCache extends EntityGroupCacheImpl<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
        ChunkCache(List<ChunkAggregateEntity> diffs) {
            super(diffs);
        }
    }

    public static class Modifiable extends GroupingEntityCacheImpl.Modifiable<
            CheckIterationEntity.Id, ChunkAggregateEntity.Id, ChunkAggregateEntity
            > implements ChunkAggregatesGroupedCache.WithCommitSupport {
        public Modifiable(
                ChunkAggregatesGroupedCacheImpl baseImpl,
                Cache<CheckIterationEntity.Id, EntityGroupCacheImpl<ChunkAggregateEntity.Id,
                        ChunkAggregateEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public Collection<ChunkAggregateEntity> getByIterationId(CheckIterationEntity.Id iterationId) {
            var result = ((ChunkAggregatesGroupedCacheImpl) this.baseImpl).getByIterationId(iterationId).stream()
                    .collect(Collectors.toMap(ChunkAggregateEntity::getId, Function.identity()));

            this.writeBuffer.values().stream()
                    .filter(x -> x.getId().getIterationId().equals(iterationId))
                    .forEach(aggregate -> result.put(aggregate.getId(), aggregate));

            return result.values();
        }

        @Override
        public Set<ChunkEntity.Id> getNotCompletedChunkIds(CheckIterationEntity.Id iterationId) {
            return this.getByIterationId(iterationId).stream()
                    .filter(x -> !x.isFinished())
                    .map(x -> x.getId().getChunkId())
                    .collect(Collectors.toSet());
        }
    }
}
