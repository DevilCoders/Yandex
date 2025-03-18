package ru.yandex.ci.observer.core.cache;

import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.storage.core.cache.AbstractModifiable;
import ru.yandex.ci.storage.core.cache.impl.StorageCacheImpl;

public class ObserverCacheImpl extends StorageCacheImpl<ObserverCache.Modifiable,
        ObserverCacheImpl.Modifiable, CiObserverDb> implements ObserverCache {

    private final ObserverSettingsCache.WithModificationSupport settings;
    private final ChecksCache.WithModificationSupport checks;
    private final CheckIterationsGroupingCache.WithModificationSupport iterationsGrouped;
    private final CheckTasksGroupingCache.WithModificationSupport tasksGrouped;
    private final CheckTaskPartitionTraceCache.WithModificationSupport traces;

    private final int maxNumberOfWritesBeforeCommit;

    public ObserverCacheImpl(
            CiObserverDb db,
            ObserverSettingsCache.WithModificationSupport settings,
            ChecksCache.WithModificationSupport checks,
            CheckIterationsGroupingCache.WithModificationSupport iterationsGrouped,
            CheckTasksGroupingCache.WithModificationSupport tasksGrouped,
            CheckTaskPartitionTraceCache.WithModificationSupport traces,
            int maxNumberOfWritesBeforeCommit
    ) {
        super(db);
        this.settings = settings;
        this.checks = checks;
        this.iterationsGrouped = iterationsGrouped;
        this.tasksGrouped = tasksGrouped;
        this.traces = traces;
        this.maxNumberOfWritesBeforeCommit = maxNumberOfWritesBeforeCommit;
    }

    @Override
    public ObserverSettingsCache settings() {
        return this.settings;
    }

    @Override
    public ChecksCache checks() {
        return this.checks;
    }

    @Override
    public CheckIterationsGroupingCache iterationsGrouped() {
        return this.iterationsGrouped;
    }

    @Override
    public CheckTasksGroupingCache tasksGrouped() {
        return this.tasksGrouped;
    }

    @Override
    public CheckTaskPartitionTraceCache traces() {
        return this.traces;
    }

    @Override
    protected Modifiable toModifiable() {
        return new Modifiable(this);
    }

    @Override
    protected void commit(Modifiable cache) {
        cache.commit();
    }

    public static class Modifiable extends AbstractModifiable implements ObserverCache.Modifiable {
        private final ObserverSettingsCache.WithCommitSupport settings;
        private final ChecksCache.WithCommitSupport checks;
        private final CheckIterationsGroupingCache.WithCommitSupport iterationsGrouped;
        private final CheckTasksGroupingCache.WithCommitSupport tasksGrouped;
        private final CheckTaskPartitionTraceCache.WithCommitSupport traces;

        public Modifiable(ObserverCacheImpl cache) {
            this.settings = (ObserverSettingsCache.WithCommitSupport)
                    register(cache.settings.toModifiable(cache.maxNumberOfWritesBeforeCommit));
            this.checks = (ChecksCache.WithCommitSupport)
                    register(cache.checks.toModifiable(cache.maxNumberOfWritesBeforeCommit));
            this.tasksGrouped = (CheckTasksGroupingCache.WithCommitSupport)
                    register(cache.tasksGrouped.toModifiable(cache.maxNumberOfWritesBeforeCommit));
            this.traces = (CheckTaskPartitionTraceCache.WithCommitSupport)
                    register(cache.traces.toModifiable(cache.maxNumberOfWritesBeforeCommit));
            this.iterationsGrouped = (CheckIterationsGroupingCache.WithCommitSupport)
                    register(cache.iterationsGrouped.toModifiable(cache.maxNumberOfWritesBeforeCommit));
        }

        @Override
        public ObserverSettingsCache.Modifiable settings() {
            return this.settings;
        }

        @Override
        public ChecksCache.Modifiable checks() {
            return this.checks;
        }

        @Override
        public CheckIterationsGroupingCache.Modifiable iterationsGrouped() {
            return this.iterationsGrouped;
        }

        @Override
        public CheckTasksGroupingCache.Modifiable tasksGrouped() {
            return this.tasksGrouped;
        }

        @Override
        public CheckTaskPartitionTraceCache.Modifiable traces() {
            return traces;
        }
    }
}
