package ru.yandex.ci.storage.core.cache;

public interface DefaultStorageCache extends StorageCoreCache<DefaultStorageCache.Modifiable> {
    interface Modifiable extends StorageCoreCache.Modifiable {

    }
}
