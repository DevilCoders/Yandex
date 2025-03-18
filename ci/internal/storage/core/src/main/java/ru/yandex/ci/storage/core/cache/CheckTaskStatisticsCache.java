package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsEntity;

public interface CheckTaskStatisticsCache
        extends EntityCache<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity> {
    interface Modifiable extends CheckTaskStatisticsCache, EntityCache.Modifiable<CheckTaskStatisticsEntity.Id,
            CheckTaskStatisticsEntity> {
    }

    interface WithModificationSupport extends CheckTaskStatisticsCache,
            EntityCache.ModificationSupport<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<CheckTaskStatisticsEntity.Id, CheckTaskStatisticsEntity> {
    }
}
