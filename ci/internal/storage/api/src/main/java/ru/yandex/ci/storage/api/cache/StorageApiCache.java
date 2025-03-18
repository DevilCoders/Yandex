package ru.yandex.ci.storage.api.cache;

import ru.yandex.ci.storage.core.cache.StorageCoreCache;

public interface StorageApiCache extends StorageCoreCache<StorageApiCache.Modifiable> {
    interface Modifiable extends StorageCoreCache.Modifiable {

    }
}
