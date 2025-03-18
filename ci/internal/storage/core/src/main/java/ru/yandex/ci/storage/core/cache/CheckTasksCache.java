package ru.yandex.ci.storage.core.cache;

import java.util.Collection;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

public interface CheckTasksCache extends EntityCache<CheckTaskEntity.Id, CheckTaskEntity> {
    Collection<CheckTaskEntity> getIterationTasks(CheckIterationEntity.Id iterationId);

    interface Modifiable extends CheckTasksCache, EntityCache.Modifiable<CheckTaskEntity.Id, CheckTaskEntity> {

    }

    interface WithModificationSupport extends CheckTasksCache, EntityCache.ModificationSupport<CheckTaskEntity.Id,
            CheckTaskEntity> {
    }

    interface WithCommitSupport extends Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<CheckTaskEntity.Id,
            CheckTaskEntity> {
    }
}
