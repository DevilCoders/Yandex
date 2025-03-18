package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.cache.ChunksGroupedCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;

public class ChunksGroupedCacheImpl extends GroupingEntityCacheImpl<Common.ChunkType,
        ChunkEntity.Id, ChunkEntity>
        implements ChunksGroupedCache, ChunksGroupedCache.WithModificationSupport {

    private static final String CACHE_NAME = "chunks";

    private final CiStorageDb db;

    public ChunksGroupedCacheImpl(CiStorageDb db, MeterRegistry meterRegistry) {
        super(
                CacheBuilder.newBuilder()
                        .maximumSize(2048)
                        .expireAfterAccess(Duration.ofSeconds(32))
                        .recordStats()
                        .build(),
                meterRegistry,
                CACHE_NAME
        );
        this.db = db;

        GuavaCacheMetrics.monitor(meterRegistry, this.cache, CACHE_NAME);
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, cache, maxNumberOfWrites);
    }

    @Override
    public List<ChunkEntity> getAll() {
        var result = new ArrayList<ChunkEntity>();
        for (var chunkType : Common.ChunkType.values()) {
            result.addAll(get(chunkType).getAll());
        }

        return result;
    }

    @Override
    protected Common.ChunkType getAggregateId(ChunkEntity.Id id) {
        return id.getChunkType();
    }

    @Override
    protected ChunkCache load(Common.ChunkType chunkType) {
        return new ChunkCache(this.db.readOnly().run(() -> this.db.chunks().getByType(chunkType)));
    }

    @Override
    protected EntityGroupCacheImpl<ChunkEntity.Id, ChunkEntity> loadForEmpty(
            Common.ChunkType chunkType
    ) {
        return new ChunkCache(new ArrayList<>());
    }

    @Override
    protected KikimrTableCi<ChunkEntity> getTable() {
        return db.chunks();
    }

    private static class ChunkCache extends EntityGroupCacheImpl<ChunkEntity.Id, ChunkEntity> {
        ChunkCache(List<ChunkEntity> diffs) {
            super(diffs);
        }
    }

    public static class Modifiable extends GroupingEntityCacheImpl.Modifiable<
            Common.ChunkType, ChunkEntity.Id, ChunkEntity
            > implements WithCommitSupport {
        public Modifiable(
                ChunksGroupedCacheImpl baseImpl,
                Cache<Common.ChunkType, EntityGroupCacheImpl<ChunkEntity.Id, ChunkEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public List<ChunkEntity> getAll() {
            var result = new ArrayList<ChunkEntity>();
            for (var chunkType : Common.ChunkType.values()) {
                result.addAll(get(chunkType).getAll());
            }

            return result;
        }
    }
}
