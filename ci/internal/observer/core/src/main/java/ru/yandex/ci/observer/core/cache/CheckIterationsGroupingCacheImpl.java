package ru.yandex.ci.observer.core.cache;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.storage.core.cache.impl.EntityGroupCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.GroupingEntityCacheImpl;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

public class CheckIterationsGroupingCacheImpl
        extends GroupingEntityCacheImpl<CheckEntity.Id, CheckIterationEntity.Id, CheckIterationEntity>
        implements CheckIterationsGroupingCache, CheckIterationsGroupingCache.WithModificationSupport {
    private static final String CACHE_NAME = "observer-iterations-grouped";

    private final CiObserverDb db;

    public CheckIterationsGroupingCacheImpl(int size, MeterRegistry meterRegistry, CiObserverDb db) {
        super(
                CacheBuilder.newBuilder()
                        .maximumSize(size)
                        .expireAfterAccess(Duration.ofHours(1))
                        .recordStats()
                        .build(),
                meterRegistry,
                CACHE_NAME
        );
        this.db = db;
        GuavaCacheMetrics.monitor(meterRegistry, cache, CACHE_NAME);
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @Override
    protected CheckEntity.Id getAggregateId(CheckIterationEntity.Id id) {
        return id.getCheckId();
    }

    @Override
    protected EntityGroupCacheImpl<CheckIterationEntity.Id, CheckIterationEntity> load(CheckEntity.Id id) {
        return new IterationsCache(db.readOnly(() -> db.iterations().findByCheck(id)));
    }

    @Override
    protected EntityGroupCacheImpl<CheckIterationEntity.Id, CheckIterationEntity> loadForEmpty(CheckEntity.Id id) {
        return new IterationsCache(new ArrayList<>());
    }

    @Override
    protected KikimrTableCi<CheckIterationEntity> getTable() {
        return db.iterations();
    }

    @Override
    public Collection<CheckIterationEntity> getAll(CheckEntity.Id aggregateId) {
        return get(aggregateId).getAll();
    }

    @Override
    public long countActive(CheckEntity.Id aggregateId) {
        return getAll(aggregateId).stream()
                .filter(i -> CheckStatusUtils.isActive(i.getStatus()))
                .count();
    }

    private static class IterationsCache extends EntityGroupCacheImpl<CheckIterationEntity.Id, CheckIterationEntity> {
        IterationsCache(List<CheckIterationEntity> entities) {
            super(entities);
        }
    }

    public static class Modifiable
            extends GroupingEntityCacheImpl.Modifiable<CheckEntity.Id, CheckIterationEntity.Id, CheckIterationEntity>
            implements WithCommitSupport {
        public Modifiable(
                GroupingEntityCacheImpl<CheckEntity.Id, CheckIterationEntity.Id, CheckIterationEntity> baseImpl,
                Cache<CheckEntity.Id, EntityGroupCacheImpl<CheckIterationEntity.Id, CheckIterationEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public Collection<CheckIterationEntity> getAll(CheckEntity.Id aggregateId) {
            return get(aggregateId).getAll();
        }

        @Override
        public long countActive(CheckEntity.Id aggregateId) {
            return getAll(aggregateId).stream()
                    .filter(i -> CheckStatusUtils.isActive(i.getStatus()))
                    .count();
        }
    }
}
