package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Collection;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.CheckTasksCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

public class CheckTasksCacheImpl extends EntityCacheImpl<CheckTaskEntity.Id, CheckTaskEntity, CiStorageDb>
        implements CheckTasksCache, CheckTasksCache.WithModificationSupport {

    public CheckTasksCacheImpl(CiStorageDb db, int maxSize, MeterRegistry meterRegistry) {
        super(
                CheckTaskEntity.class,
                // Keep expire time low, to prevent processing of finished results from main stream
                EntityCacheImpl.createDefault(maxSize, Duration.ofMinutes(1)),
                db,
                meterRegistry,
                "check-tasks"
        );
        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckTaskEntity> getTable() {
        return this.db.checkTasks();
    }

    @Override
    public EntityCache.Modifiable.CacheWithCommitSupport<CheckTaskEntity.Id, CheckTaskEntity> toModifiable(
            int maxNumberOfWrites
    ) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @Override
    public Collection<CheckTaskEntity> getIterationTasks(CheckIterationEntity.Id iterationId) {
        return this.db.checkTasks().getByIteration(iterationId);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<CheckTaskEntity.Id, CheckTaskEntity, CiStorageDb>
            implements CheckTasksCache.WithCommitSupport {
        public Modifiable(
                CheckTasksCacheImpl baseImpl, Cache<CheckTaskEntity.Id, Optional<CheckTaskEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public Collection<CheckTaskEntity> getIterationTasks(CheckIterationEntity.Id iterationId) {
            var tasks = ((CheckTasksCacheImpl) this.baseImpl).getIterationTasks(iterationId);
            tasks.forEach(task -> this.writeBuffer.put(task.getId(), Optional.of(task)));
            return tasks;
        }
    }
}
