package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.TestsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class TestsCacheImpl extends EntityCacheImpl<TestStatusEntity.Id, TestStatusEntity, CiStorageDb>
        implements TestsCache, TestsCache.WithModificationSupport {
    public TestsCacheImpl(
            CiStorageDb db,
            int size,
            MeterRegistry meterRegistry
    ) {
        super(
                TestStatusEntity.class,
                EntityCacheImpl.createDefault(size, Duration.ofHours(1)),
                db,
                meterRegistry,
                "tests",
                TestStatusEntity.defaultProvider()
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<TestStatusEntity> getTable() {
        return this.db.tests();
    }

    @Override
    public TestsCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<TestStatusEntity.Id, TestStatusEntity, CiStorageDb>
            implements TestsCache.WithCommitSupport {

        public Modifiable(
                TestsCacheImpl baseImpl, Cache<TestStatusEntity.Id, Optional<TestStatusEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // aggressive caching
            );
        }
    }
}
