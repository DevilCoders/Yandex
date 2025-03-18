package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;

public interface LargeTasksCache extends EntityCache<LargeTaskEntity.Id, LargeTaskEntity> {

    interface Modifiable extends LargeTasksCache,
            EntityCache.Modifiable<LargeTaskEntity.Id, LargeTaskEntity> {
    }

    interface WithModificationSupport extends LargeTasksCache,
            ModificationSupport<LargeTaskEntity.Id, LargeTaskEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<LargeTaskEntity.Id, LargeTaskEntity> {
    }
}
