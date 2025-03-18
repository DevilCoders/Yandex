package ru.yandex.ci.storage.core.cache.impl;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.apache.commons.lang3.tuple.Pair;

import ru.yandex.ci.storage.core.cache.AbstractModifiable;
import ru.yandex.ci.storage.core.cache.CheckMergeRequirementsCache;
import ru.yandex.ci.storage.core.cache.CheckTasksCache;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.ChunksGroupedCache;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.LargeTasksCache;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.cache.StorageCustomCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;

public abstract class StorageCoreCacheImpl<M extends StorageCoreCache.Modifiable, ModifiableInner extends M>
        extends StorageCacheImpl<M, ModifiableInner, CiStorageDb>
        implements StorageCoreCache<M> {

    private final List<EntityCache.ModificationSupport<?, ?>> caches;
    private final Map<Class<?>, EntityCache.ModificationSupport<?, ?>> cachesByInterface;
    private final Map<Class<?>, StorageCustomCache> customCachesByInterface;

    private final int maxNumberOfWritesBeforeCommit;

    public StorageCoreCacheImpl(
            CiStorageDb db,
            List<EntityCache.ModificationSupport<?, ?>> caches,
            int maxNumberOfWritesBeforeCommit
    ) {
        this(db, caches, List.of(), maxNumberOfWritesBeforeCommit);
    }

    public StorageCoreCacheImpl(
            CiStorageDb db,
            List<EntityCache.ModificationSupport<?, ?>> caches,
            List<StorageCustomCache> customCaches,
            int maxNumberOfWritesBeforeCommit
    ) {
        super(db);
        this.caches = caches;
        this.cachesByInterface = caches.stream()
                .flatMap(x -> Arrays.stream(x.getClass().getInterfaces()).map(i -> Pair.of(i, x)))
                .collect(Collectors.toMap(Pair::getLeft, Pair::getRight));
        this.customCachesByInterface = customCaches.stream()
                .flatMap(x -> Arrays.stream(x.getClass().getInterfaces()).map(i -> Pair.of(i, x)))
                .collect(Collectors.toMap(Pair::getLeft, Pair::getRight));
        this.maxNumberOfWritesBeforeCommit = maxNumberOfWritesBeforeCommit;
    }

    @Override
    public IterationsCache iterations() {
        return get(IterationsCache.class);
    }

    @Override
    public CheckTasksCache checkTasks() {
        return get(CheckTasksCache.class);
    }

    @Override
    public LargeTasksCache largeTasks() {
        return get(LargeTasksCache.class);
    }

    @Override
    public SettingsCache settings() {
        return get(SettingsCache.class);
    }

    @Override
    public ChecksCache checks() {
        return get(ChecksCache.class);
    }

    @Override
    public ChunksGroupedCache chunks() {
        return get(ChunksGroupedCache.class);
    }

    @Override
    public CheckMergeRequirementsCache mergeRequirements() {
        return get(CheckMergeRequirementsCache.class);
    }

    @Override
    protected void commit(ModifiableInner cache) {
        cache.commit();
    }

    @SuppressWarnings("unchecked")
    protected <T> T get(Class<T> type) {
        Preconditions.checkState(
                this.hasNoActiveTransaction(), "Cache modification in progress. Use write cache for access."
        );
        return checkProvided(type, (T) cachesByInterface.get(type));
    }

    @SuppressWarnings("unchecked")
    protected <T> T getCustom(Class<T> type) {
        return checkProvided(type, (T) customCachesByInterface.get(type));
    }

    private <T> T checkProvided(Class<T> type, @Nullable T result) {
        Preconditions.checkNotNull(result, "Cache not provided: " + type.getSimpleName());
        return result;
    }

    public class Modifiable extends AbstractModifiable implements StorageCoreCache.Modifiable {
        private final Map<Class<?>, EntityCache.Modifiable.CacheWithCommitSupport<?, ?>> caches;

        public Modifiable(StorageCoreCacheImpl<M, ModifiableInner> original) {
            this.caches = original.caches.stream()
                    .map(cache -> register(cache.toModifiable(original.maxNumberOfWritesBeforeCommit)))
                    .flatMap(x -> Arrays.stream(x.getClass().getInterfaces()).map(i -> Pair.of(i, x)))
                    .collect(Collectors.toMap(Pair::getLeft, Pair::getRight));
        }

        @Override
        public ChecksCache.Modifiable checks() {
            return get(ChecksCache.WithCommitSupport.class);
        }

        @Override
        public ChunksGroupedCache.Modifiable chunks() {
            return get(ChunksGroupedCache.WithCommitSupport.class);
        }

        @Override
        public CheckMergeRequirementsCache.Modifiable mergeRequirements() {
            return get(CheckMergeRequirementsCache.WithCommitSupport.class);
        }

        @Override
        public IterationsCache.Modifiable iterations() {
            return get(IterationsCache.WithCommitSupport.class);
        }

        @Override
        public CheckTasksCache.Modifiable checkTasks() {
            return get(CheckTasksCache.WithCommitSupport.class);
        }

        @Override
        public LargeTasksCache.Modifiable largeTasks() {
            return get(LargeTasksCache.WithCommitSupport.class);
        }

        @Override
        public SettingsCache settings() {
            return StorageCoreCacheImpl.this.settings();
        }

        @Override
        public void invalidateAll() {
            super.invalidateAll();
            StorageCoreCacheImpl.this.customCachesByInterface.values().forEach(StorageCustomCache::invalidateAll);
        }

        @SuppressWarnings("unchecked")
        protected <T extends EntityCache.Modifiable.CacheWithCommitSupport<?, ?>> T get(Class<T> type) {
            return (T) checkCacheProvided(type, caches.get(type));
        }

        private <T> EntityCache.Modifiable.CacheWithCommitSupport<?, ?> checkCacheProvided(
                Class<T> type, EntityCache.Modifiable.CacheWithCommitSupport<?, ?> result
        ) {
            Preconditions.checkNotNull(result, "Cache not provided: " + type);
            return result;
        }
    }
}
