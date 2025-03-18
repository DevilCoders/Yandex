package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.CheckMergeRequirementsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;

public class CheckMergeRequirementsCacheImpl extends EntityCacheImpl<
        CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity, CiStorageDb
        >
        implements CheckMergeRequirementsCache, CheckMergeRequirementsCache.WithModificationSupport {

    public CheckMergeRequirementsCacheImpl(
            CiStorageDb db,
            int maxSize,
            MeterRegistry meterRegistry
    ) {
        super(
                CheckMergeRequirementsEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "check-merge-requirements"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckMergeRequirementsEntity> getTable() {
        return this.db.checkMergeRequirements();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<
            CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity, CiStorageDb
            > implements WithCommitSupport {

        public Modifiable(
                CheckMergeRequirementsCacheImpl baseImpl,
                Cache<CheckMergeRequirementsEntity.Id, Optional<CheckMergeRequirementsEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // possibly must be true
            );
        }
    }
}
