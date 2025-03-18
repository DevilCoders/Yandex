package ru.yandex.ci.storage.core.cache;


import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;

public interface TestStatisticsCache extends EntityCache<TestStatisticsEntity.Id, TestStatisticsEntity> {
    interface Modifiable extends TestStatisticsCache, EntityCache.Modifiable<TestStatisticsEntity.Id,
            TestStatisticsEntity> {
    }

    interface WithModificationSupport
            extends TestStatisticsCache, ModificationSupport<TestStatisticsEntity.Id, TestStatisticsEntity> {
    }

    interface WithCommitSupport extends
            Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<TestStatisticsEntity.Id, TestStatisticsEntity> {
    }
}
