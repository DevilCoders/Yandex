package ru.yandex.ci.storage.reader.cache.impl;

import java.util.List;

import ru.yandex.ci.storage.core.cache.CheckTaskStatisticsCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.SkippedChecksCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.cache.impl.StorageCoreCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.reader.cache.ChunkAggregatesGroupedCache;
import ru.yandex.ci.storage.reader.cache.ReaderCache;

public class ReaderCacheImpl extends
        StorageCoreCacheImpl<ReaderCache.Modifiable, ReaderCacheImpl.Modifiable>
        implements ReaderCache {

    public ReaderCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                           int maxNumberOfWritesBeforeCommit) {
        super(db, caches, maxNumberOfWritesBeforeCommit);
    }

    public ReaderCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                           List<StorageCustomCache> customCaches, int maxNumberOfWritesBeforeCommit) {
        super(db, caches, customCaches, maxNumberOfWritesBeforeCommit);
    }

    @Override
    public ChunkAggregatesGroupedCache chunkAggregatesGroupedByIteration() {
        return get(ChunkAggregatesGroupedCache.class);
    }

    @Override
    public SkippedChecksCache skippedChecks() {
        return get(SkippedChecksCache.class);
    }


    @Override
    public CheckTaskStatisticsCache taskStatistics() {
        return get(CheckTaskStatisticsCache.class);
    }

    @Override
    protected Modifiable toModifiable() {
        return new Modifiable(this);
    }

    public class Modifiable
            extends StorageCoreCacheImpl<ReaderCache.Modifiable, Modifiable>.Modifiable
            implements ReaderCache.Modifiable {

        public Modifiable(StorageCoreCacheImpl<ReaderCache.Modifiable, Modifiable> original) {
            super(original);
        }

        @Override
        public ChunkAggregatesGroupedCache.Modifiable chunkAggregatesGroupedByIteration() {
            return super.get(ChunkAggregatesGroupedCache.WithCommitSupport.class);
        }

        @Override
        public CheckTaskStatisticsCache.Modifiable taskStatistics() {
            return get(CheckTaskStatisticsCache.WithCommitSupport.class);
        }


        @Override
        public SkippedChecksCache.Modifiable skippedChecks() {
            return get(SkippedChecksCache.WithCommitSupport.class);
        }
    }

}
