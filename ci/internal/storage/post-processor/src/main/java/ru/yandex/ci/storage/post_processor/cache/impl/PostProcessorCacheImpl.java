package ru.yandex.ci.storage.post_processor.cache.impl;

import java.util.List;

import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.cache.TestStatisticsCache;
import ru.yandex.ci.storage.core.cache.TestsCache;
import ru.yandex.ci.storage.core.cache.impl.StorageCoreCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCache;

public class PostProcessorCacheImpl extends
        StorageCoreCacheImpl<PostProcessorCache.Modifiable, PostProcessorCacheImpl.Modifiable>
        implements PostProcessorCache {

    public PostProcessorCacheImpl(
            CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
            List<StorageCustomCache> customCaches, int maxNumberOfWritesBeforeCommit
    ) {
        super(db, caches, customCaches, maxNumberOfWritesBeforeCommit);
    }

    @Override
    protected PostProcessorCacheImpl.Modifiable toModifiable() {
        return new Modifiable(this);
    }

    @Override
    public TestHistoryCache testHistory() {
        return this.getCustom(TestHistoryCache.class);
    }

    @Override
    public TestsCache tests() {
        return get(TestsCache.class);
    }

    @Override
    public TestStatisticsCache testStatistics() {
        return get(TestStatisticsCache.class);
    }

    public class Modifiable
            extends StorageCoreCacheImpl<PostProcessorCache.Modifiable, PostProcessorCacheImpl.Modifiable>.Modifiable
            implements PostProcessorCache.Modifiable {

        public Modifiable(
                StorageCoreCacheImpl<PostProcessorCache.Modifiable, PostProcessorCacheImpl.Modifiable> original
        ) {
            super(original);
        }

        @Override
        public TestsCache.Modifiable tests() {
            return get(TestsCache.WithCommitSupport.class);
        }

        @Override
        public TestStatisticsCache.Modifiable testStatistics() {
            return get(TestStatisticsCache.WithCommitSupport.class);
        }

    }
}
