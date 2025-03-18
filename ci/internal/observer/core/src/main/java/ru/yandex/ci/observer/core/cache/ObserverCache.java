package ru.yandex.ci.observer.core.cache;

import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCache;

public interface ObserverCache extends StorageCache<ObserverCache.Modifiable, CiObserverDb> {
    ChecksCache checks();

    ObserverSettingsCache settings();

    CheckIterationsGroupingCache iterationsGrouped();

    CheckTasksGroupingCache tasksGrouped();

    CheckTaskPartitionTraceCache traces();

    interface Modifiable extends EntityCache.Modifiable.CommitSupport {
        ChecksCache.Modifiable checks();

        ObserverSettingsCache.Modifiable settings();

        CheckIterationsGroupingCache.Modifiable iterationsGrouped();

        CheckTasksGroupingCache.Modifiable tasksGrouped();

        CheckTaskPartitionTraceCache.Modifiable traces();

        void invalidateAll();
    }
}
