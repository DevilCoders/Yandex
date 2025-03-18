package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.CheckTaskStatisticsCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsEntity;

public class CheckTaskStatisticsCacheImpl
        extends EntityCacheImpl<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity, CiStorageDb>
        implements CheckTaskStatisticsCache, CheckTaskStatisticsCache.WithModificationSupport {

    public CheckTaskStatisticsCacheImpl(
            CiStorageDb db,
            int maxSize,
            MeterRegistry meterRegistry
    ) {
        super(
                CheckTaskStatisticsEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofHours(1)),
                db,
                meterRegistry,
                "task-statistics",
                CheckTaskStatisticsEntity.defaultProvider()
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckTaskStatisticsEntity> getTable() {
        return this.db.checkTaskStatistics();
    }

    @Override
    public EntityCache.Modifiable.CacheWithCommitSupport<
            CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity
            > toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable
            extends ModifiableEntityCacheImpl<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity, CiStorageDb>
            implements CheckTaskStatisticsCache.WithCommitSupport {

        public Modifiable(
                CheckTaskStatisticsCacheImpl baseImpl, Cache<CheckTaskStatisticsEntity.Id,
                Optional<CheckTaskStatisticsEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // Entities separated by hostname in id
            );
        }
    }
}
