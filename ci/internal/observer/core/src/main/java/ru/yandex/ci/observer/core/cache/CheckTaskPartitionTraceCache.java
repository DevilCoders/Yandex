package ru.yandex.ci.observer.core.cache;

import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.storage.core.cache.EntityCache;

public interface CheckTaskPartitionTraceCache
        extends EntityCache<CheckTaskPartitionTraceEntity.Id, CheckTaskPartitionTraceEntity> {
    interface Modifiable extends
            CheckTaskPartitionTraceCache,
            EntityCache.Modifiable<CheckTaskPartitionTraceEntity.Id, CheckTaskPartitionTraceEntity> {
    }

    interface WithModificationSupport extends CheckTaskPartitionTraceCache {
        WithCommitSupport toModifiable(int maxNumberOfWrites);
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<CheckTaskPartitionTraceEntity.Id,
                    CheckTaskPartitionTraceEntity> {
    }
}
