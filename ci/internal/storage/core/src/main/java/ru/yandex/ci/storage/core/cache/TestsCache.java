package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public interface TestsCache extends EntityCache<TestStatusEntity.Id, TestStatusEntity> {
    interface Modifiable extends TestsCache, EntityCache.Modifiable<TestStatusEntity.Id, TestStatusEntity> {
    }

    interface WithModificationSupport
            extends TestsCache, EntityCache.ModificationSupport<TestStatusEntity.Id, TestStatusEntity> {
    }

    interface WithCommitSupport extends
            Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<TestStatusEntity.Id, TestStatusEntity> {
    }
}
