package ru.yandex.ci.observer.core.cache;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.cache.EntityCache;

public interface ChecksCache extends EntityCache<CheckEntity.Id, CheckEntity> {
    interface Modifiable extends ChecksCache, EntityCache.Modifiable<CheckEntity.Id, CheckEntity> {
    }

    interface WithModificationSupport extends ChecksCache, EntityCache.ModificationSupport<CheckEntity.Id,
            CheckEntity> {
    }

    interface WithCommitSupport extends Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<CheckEntity.Id,
            CheckEntity> {
    }
}
