package ru.yandex.ci.storage.shard.cache.impl;

import java.util.List;

import ru.yandex.ci.storage.core.cache.CheckTextSearchCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.cache.impl.StorageCoreCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.shard.cache.ChunkAggregatesCache;
import ru.yandex.ci.storage.shard.cache.MuteStatusCache;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.storage.shard.cache.StrongModeCache;
import ru.yandex.ci.storage.shard.cache.TestDiffsCache;

public class ShardCacheImpl extends
        StorageCoreCacheImpl<ShardCache.Modifiable, ShardCacheImpl.Modifiable>
        implements ShardCache {

    public ShardCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                          int maxNumberOfWritesBeforeCommit) {
        super(db, caches, maxNumberOfWritesBeforeCommit);
    }

    public ShardCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                          List<StorageCustomCache> customCaches, int maxNumberOfWritesBeforeCommit) {
        super(db, caches, customCaches, maxNumberOfWritesBeforeCommit);
    }

    @Override
    public ChunkAggregatesCache chunkAggregates() {
        return get(ChunkAggregatesCache.class);
    }

    @Override
    public TestDiffsCache testDiffs() {
        return get(TestDiffsCache.class);
    }

    @Override
    public MuteStatusCache muteStatus() {
        return getCustom(MuteStatusCache.class);
    }

    @Override
    public StrongModeCache strongModeCache() {
        return getCustom(StrongModeCache.class);
    }

    @Override
    public CheckTextSearchCache checkTextSearch() {
        return getCustom(CheckTextSearchCache.class);
    }

    @Override
    protected ShardCacheImpl.Modifiable toModifiable() {
        return new Modifiable(this);
    }

    public class Modifiable
            extends StorageCoreCacheImpl<ShardCache.Modifiable, ShardCacheImpl.Modifiable>.Modifiable
            implements ShardCache.Modifiable {

        public Modifiable(StorageCoreCacheImpl<ShardCache.Modifiable, ShardCacheImpl.Modifiable> original) {
            super(original);
        }

        @Override
        public ChunkAggregatesCache.Modifiable chunkAggregates() {
            return super.get(ChunkAggregatesCache.WithCommitSupport.class);
        }

        @Override
        public TestDiffsCache.Modifiable testDiffs() {
            return super.get(TestDiffsCache.WithCommitSupport.class);
        }

        @Override
        public MuteStatusCache muteStatus() {
            return ShardCacheImpl.this.muteStatus();
        }

        @Override
        public CheckTextSearchCache checkTextSearch() {
            return ShardCacheImpl.this.checkTextSearch();
        }
    }

}
