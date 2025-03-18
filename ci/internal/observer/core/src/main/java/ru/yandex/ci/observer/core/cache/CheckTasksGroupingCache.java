package ru.yandex.ci.observer.core.cache;

import java.util.Collection;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.GroupingEntityCache;

public interface CheckTasksGroupingCache
        extends GroupingEntityCache<CheckIterationEntity.Id, CheckTaskEntity.Id, CheckTaskEntity> {
    Collection<CheckTaskEntity> getAll(CheckIterationEntity.Id aggregateId);

    interface Modifiable extends CheckTasksGroupingCache,
            GroupingEntityCache.Modifiable<CheckIterationEntity.Id, CheckTaskEntity.Id, CheckTaskEntity> {
    }

    interface WithModificationSupport extends CheckTasksGroupingCache {
        WithCommitSupport toModifiable(int maxNumberOfWrites);
    }

    interface WithCommitSupport
            extends CheckTasksGroupingCache.Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<CheckTaskEntity.Id, CheckTaskEntity> {
    }
}
