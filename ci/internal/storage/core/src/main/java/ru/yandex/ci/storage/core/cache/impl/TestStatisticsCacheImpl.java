package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.TestStatisticsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;

public class TestStatisticsCacheImpl extends EntityCacheImpl<TestStatisticsEntity.Id, TestStatisticsEntity, CiStorageDb>
        implements TestStatisticsCache, TestStatisticsCache.WithModificationSupport {
    public TestStatisticsCacheImpl(
            CiStorageDb db,
            int size,
            MeterRegistry meterRegistry
    ) {
        super(
                TestStatisticsEntity.class,
                EntityCacheImpl.createDefault(size, Duration.ofHours(1)),
                db,
                meterRegistry,
                "test-statistics",
                TestStatisticsEntity.defaultProvider()
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<TestStatisticsEntity> getTable() {
        return this.db.testStatistics();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<TestStatisticsEntity.Id, TestStatisticsEntity,
            CiStorageDb>
            implements WithCommitSupport {

        public Modifiable(
                TestStatisticsCacheImpl baseImpl, Cache<TestStatisticsEntity.Id, Optional<TestStatisticsEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // aggressive caching
            );
        }
    }
}
