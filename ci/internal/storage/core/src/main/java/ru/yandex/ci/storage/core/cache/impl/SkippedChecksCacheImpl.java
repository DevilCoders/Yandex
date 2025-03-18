package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.SkippedChecksCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckEntity;

public class SkippedChecksCacheImpl extends EntityCacheImpl<SkippedCheckEntity.Id, SkippedCheckEntity, CiStorageDb>
        implements SkippedChecksCache, SkippedChecksCache.WithModificationSupport {

    public SkippedChecksCacheImpl(
            CiStorageDb db,
            int maxSize,
            MeterRegistry meterRegistry
    ) {
        super(
                SkippedCheckEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "skipped-checks"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<SkippedCheckEntity> getTable() {
        return this.db.skippedChecks();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<SkippedCheckEntity.Id, SkippedCheckEntity,
            CiStorageDb>
            implements WithCommitSupport {

        public Modifiable(
                SkippedChecksCacheImpl baseImpl,
                Cache<SkippedCheckEntity.Id, Optional<SkippedCheckEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false
            );
        }
    }
}
