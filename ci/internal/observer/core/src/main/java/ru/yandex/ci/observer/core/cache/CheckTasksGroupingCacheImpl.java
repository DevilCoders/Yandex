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
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.cache.impl.EntityGroupCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.GroupingEntityCacheImpl;

public class CheckTasksGroupingCacheImpl
        extends GroupingEntityCacheImpl<CheckIterationEntity.Id, CheckTaskEntity.Id, CheckTaskEntity>
        implements CheckTasksGroupingCache, CheckTasksGroupingCache.WithModificationSupport {
    private static final String CACHE_NAME = "observer-tasks-grouped";

    private final CiObserverDb db;

    public CheckTasksGroupingCacheImpl(int size, MeterRegistry meterRegistry, CiObserverDb db) {
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
    public Collection<CheckTaskEntity> getAll(CheckIterationEntity.Id aggregateId) {
        return get(aggregateId).getAll();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @Override
    protected CheckIterationEntity.Id getAggregateId(CheckTaskEntity.Id id) {
        return id.getIterationId();
    }

    @Override
    protected EntityGroupCacheImpl<CheckTaskEntity.Id, CheckTaskEntity> load(CheckIterationEntity.Id id) {
        return new TasksCache(db.readOnly(() -> db.tasks().getByIteration(id)));
    }

    @Override
    protected EntityGroupCacheImpl<CheckTaskEntity.Id, CheckTaskEntity> loadForEmpty(CheckIterationEntity.Id id) {
        return new TasksCache(new ArrayList<>());
    }

    @Override
    protected KikimrTableCi<CheckTaskEntity> getTable() {
        return db.tasks();
    }

    private static class TasksCache extends EntityGroupCacheImpl<CheckTaskEntity.Id, CheckTaskEntity> {
        TasksCache(List<CheckTaskEntity> entities) {
            super(entities);
        }
    }

    public static class Modifiable
            extends GroupingEntityCacheImpl.Modifiable<CheckIterationEntity.Id, CheckTaskEntity.Id, CheckTaskEntity>
            implements WithCommitSupport {
        public Modifiable(
                GroupingEntityCacheImpl<CheckIterationEntity.Id, CheckTaskEntity.Id, CheckTaskEntity> baseImpl,
                Cache<CheckIterationEntity.Id, EntityGroupCacheImpl<CheckTaskEntity.Id, CheckTaskEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public Collection<CheckTaskEntity> getAll(CheckIterationEntity.Id aggregateId) {
            return get(aggregateId).getAll();
        }
    }
}
