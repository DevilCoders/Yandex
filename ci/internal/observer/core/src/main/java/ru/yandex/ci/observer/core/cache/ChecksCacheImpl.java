package ru.yandex.ci.observer.core.cache;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.cache.impl.EntityCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ModifiableEntityCacheImpl;

public class ChecksCacheImpl extends EntityCacheImpl<CheckEntity.Id, CheckEntity, CiObserverDb>
        implements ChecksCache, ChecksCache.WithModificationSupport {

    public ChecksCacheImpl(
            CiObserverDb db,
            int maxSize,
            MeterRegistry meterRegistry
    ) {
        super(
                CheckEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "observer-checks"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckEntity> getTable() {
        return this.db.checks();
    }

    @Override
    public ChecksCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<CheckEntity.Id, CheckEntity, CiObserverDb>
            implements ChecksCache.WithCommitSupport {

        public Modifiable(ChecksCacheImpl baseImpl, Cache<CheckEntity.Id, Optional<CheckEntity>> cache,
                          int maxNumberOfWrites) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // possibly must be true
            );
        }
    }
}
