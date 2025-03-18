package ru.yandex.ci.observer.core.cache;

import java.util.Collection;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.GroupingEntityCache;

public interface CheckIterationsGroupingCache
        extends GroupingEntityCache<CheckEntity.Id, CheckIterationEntity.Id, CheckIterationEntity> {
    Collection<CheckIterationEntity> getAll(CheckEntity.Id aggregateId);
    long countActive(CheckEntity.Id aggregateId);

    interface Modifiable extends CheckIterationsGroupingCache,
            GroupingEntityCache.Modifiable<CheckEntity.Id, CheckIterationEntity.Id, CheckIterationEntity> {
    }

    interface WithModificationSupport extends CheckIterationsGroupingCache {
        WithCommitSupport toModifiable(int maxNumberOfWrites);
    }

    interface WithCommitSupport
            extends CheckIterationsGroupingCache.Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<CheckIterationEntity.Id, CheckIterationEntity> {
    }
}
