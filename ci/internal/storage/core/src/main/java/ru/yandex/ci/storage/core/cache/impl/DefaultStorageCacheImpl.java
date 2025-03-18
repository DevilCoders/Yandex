package ru.yandex.ci.storage.core.cache.impl;

import java.util.List;

import ru.yandex.ci.storage.core.cache.DefaultStorageCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;

public class DefaultStorageCacheImpl extends
        StorageCoreCacheImpl<DefaultStorageCache.Modifiable, DefaultStorageCacheImpl.Modifiable>
        implements DefaultStorageCache {

    public DefaultStorageCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                                   int maxNumberOfWritesBeforeCommit) {
        super(db, caches, maxNumberOfWritesBeforeCommit);
    }

    public DefaultStorageCacheImpl(CiStorageDb db, List<EntityCache.ModificationSupport<?, ?>> caches,
                                   List<StorageCustomCache> customCaches, int maxNumberOfWritesBeforeCommit) {
        super(db, caches, customCaches, maxNumberOfWritesBeforeCommit);
    }

    @Override
    protected Modifiable toModifiable() {
        return new Modifiable(this);
    }

    public class Modifiable
            extends StorageCoreCacheImpl<DefaultStorageCache.Modifiable, DefaultStorageCacheImpl.Modifiable>.Modifiable
            implements DefaultStorageCache.Modifiable {

        public Modifiable(StorageCoreCacheImpl<DefaultStorageCache.Modifiable, Modifiable> original) {
            super(original);
        }
    }

}
