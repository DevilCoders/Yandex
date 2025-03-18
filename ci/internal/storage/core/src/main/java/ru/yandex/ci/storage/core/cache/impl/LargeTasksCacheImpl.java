package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.LargeTasksCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;

public class LargeTasksCacheImpl extends EntityCacheImpl<LargeTaskEntity.Id, LargeTaskEntity, CiStorageDb>
        implements LargeTasksCache, LargeTasksCache.WithModificationSupport {

    public LargeTasksCacheImpl(CiStorageDb db, int maxSize, MeterRegistry meterRegistry) {
        super(
                LargeTaskEntity.class,
                // Keep expire time low, to prevent processing of finished results from main stream
                EntityCacheImpl.createDefault(maxSize, Duration.ofMinutes(1)),
                db,
                meterRegistry,
                "check-tasks"
        );
        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<LargeTaskEntity> getTable() {
        return this.db.largeTasks();
    }

    @Override
    public EntityCache.Modifiable.CacheWithCommitSupport<LargeTaskEntity.Id, LargeTaskEntity> toModifiable(
            int maxNumberOfWrites
    ) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<LargeTaskEntity.Id, LargeTaskEntity, CiStorageDb>
            implements WithCommitSupport {
        public Modifiable(
                LargeTasksCacheImpl baseImpl, Cache<LargeTaskEntity.Id, Optional<LargeTaskEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // possibly must be true
            );
        }
    }
}
