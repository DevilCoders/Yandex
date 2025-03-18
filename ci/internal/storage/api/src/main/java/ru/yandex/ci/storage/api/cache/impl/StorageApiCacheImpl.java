package ru.yandex.ci.storage.api.cache.impl;

import java.util.List;

import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.cache.impl.StorageCoreCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;

public class StorageApiCacheImpl extends
        StorageCoreCacheImpl<StorageApiCache.Modifiable, StorageApiCacheImpl.Modifiable>
        implements StorageApiCache {

    public StorageApiCacheImpl(
            CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
            List<StorageCustomCache> customCaches, int maxNumberOfWritesBeforeCommit
    ) {
        super(db, caches, customCaches, maxNumberOfWritesBeforeCommit);
    }

    @Override
    protected Modifiable toModifiable() {
        return new Modifiable(this);
    }

    public class Modifiable
            extends StorageCoreCacheImpl<StorageApiCache.Modifiable, Modifiable>.Modifiable
            implements StorageApiCache.Modifiable {

        public Modifiable(
                StorageCoreCacheImpl<StorageApiCache.Modifiable, Modifiable> original
        ) {
            super(original);
        }
    }
}
