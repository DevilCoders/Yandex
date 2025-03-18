package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.CiStorageDb;

public interface StorageCoreCache<T extends StorageCoreCache.Modifiable>
        extends StorageCache<T, CiStorageDb> {

    ChecksCache checks();

    ChunksGroupedCache chunks();

    CheckMergeRequirementsCache mergeRequirements();

    IterationsCache iterations();

    CheckTasksCache checkTasks();

    LargeTasksCache largeTasks();

    SettingsCache settings();

    interface Modifiable extends EntityCache.Modifiable.CommitSupport {
        ChecksCache.Modifiable checks();

        ChunksGroupedCache.Modifiable chunks();

        CheckMergeRequirementsCache.Modifiable mergeRequirements();

        IterationsCache.Modifiable iterations();

        CheckTasksCache.Modifiable checkTasks();

        LargeTasksCache.Modifiable largeTasks();

        SettingsCache settings();

        void invalidateAll();
    }
}
