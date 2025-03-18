package ru.yandex.ci.observer.core.cache;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.storage.core.cache.impl.EntityCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ModifiableEntityCacheImpl;

public class CheckTaskPartitionTraceCacheImpl
        extends EntityCacheImpl<CheckTaskPartitionTraceEntity.Id, CheckTaskPartitionTraceEntity, CiObserverDb>
        implements CheckTaskPartitionTraceCache, CheckTaskPartitionTraceCache.WithModificationSupport {

    public CheckTaskPartitionTraceCacheImpl(CiObserverDb db, int maxSize, MeterRegistry meterRegistry) {
        super(
                CheckTaskPartitionTraceEntity.class,
                EntityCacheImpl.createDefault(maxSize, Duration.ofMinutes(30)),
                db,
                meterRegistry,
                "observer-traces"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckTaskPartitionTraceEntity> getTable() {
        return this.db.traces();
    }

    @Override
    public CheckTaskPartitionTraceCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    public static class Modifiable
            extends ModifiableEntityCacheImpl<CheckTaskPartitionTraceEntity.Id,
                                              CheckTaskPartitionTraceEntity,
                                              CiObserverDb>
            implements CheckTaskPartitionTraceCache.WithCommitSupport {

        public Modifiable(
                CheckTaskPartitionTraceCacheImpl baseImpl,
                Cache<CheckTaskPartitionTraceEntity.Id, Optional<CheckTaskPartitionTraceEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(
                    baseImpl, cache, maxNumberOfWrites,
                    false // possibly must be true
            );
        }
    }
}
