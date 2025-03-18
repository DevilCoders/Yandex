package ru.yandex.ci.storage.shard.cache.impl;

import java.time.Duration;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.impl.EntityCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ModifiableEntityCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.shard.cache.ChunkAggregatesCache;

@Slf4j
public class ChunkAggregatesCacheImpl
        extends EntityCacheImpl<ChunkAggregateEntity.Id, ChunkAggregateEntity, CiStorageDb>
        implements ChunkAggregatesCache, ChunkAggregatesCache.WithModificationSupport {

    public ChunkAggregatesCacheImpl(CiStorageDb db, int maxSize, MeterRegistry meterRegistry) {
        super(
                ChunkAggregateEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "chunk-aggregates",
                ChunkAggregateEntity.defaultProvider()
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<ChunkAggregateEntity> getTable() {
        return this.db.chunkAggregates();
    }

    @Override
    public ChunkAggregatesCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable
            extends ModifiableEntityCacheImpl<ChunkAggregateEntity.Id, ChunkAggregateEntity, CiStorageDb>
            implements ChunkAggregatesCache.WithCommitSupport {

        public Modifiable(ChunkAggregatesCacheImpl baseImpl, Cache<ChunkAggregateEntity.Id,
                Optional<ChunkAggregateEntity>> cache, int maxNumberOfWrites) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // aggressive caching
            );
        }

        @Override
        public void invalidate(Set<ChunkEntity.Id> chunks) {
            var aggregatesToRemove = this.cache.asMap().keySet().stream()
                    .filter(x -> chunks.contains(x.getChunkId()))
                    .collect(Collectors.toSet());

            log.info("Invalidating {} aggregates", aggregatesToRemove.size());

            this.cache.invalidateAll(aggregatesToRemove);
        }
    }
}
