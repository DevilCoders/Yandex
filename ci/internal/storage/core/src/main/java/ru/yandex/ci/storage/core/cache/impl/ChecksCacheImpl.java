package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

public class ChecksCacheImpl extends EntityCacheImpl<CheckEntity.Id, CheckEntity, CiStorageDb>
        implements ChecksCache, ChecksCache.WithModificationSupport {

    public ChecksCacheImpl(
            CiStorageDb db,
            int maxSize,
            MeterRegistry meterRegistry
    ) {
        super(
                CheckEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "checks"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckEntity> getTable() {
        return this.db.checks();
    }

    @Override
    public EntityCache.Modifiable.CacheWithCommitSupport<CheckEntity.Id, CheckEntity> toModifiable(
            int maxNumberOfWrites
    ) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<CheckEntity.Id, CheckEntity, CiStorageDb>
            implements ChecksCache.WithCommitSupport {
        public Modifiable(
                ChecksCacheImpl baseImpl, Cache<CheckEntity.Id, Optional<CheckEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }
    }
}
