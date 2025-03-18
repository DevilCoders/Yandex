package ru.yandex.ci.storage.core.cache;

import java.util.ArrayList;
import java.util.List;

import yandex.cloud.repository.db.Entity;

public abstract class AbstractModifiable implements EntityCache.Modifiable.CommitSupport {
    private static final int DEFAULT_CACHE_SIZE = 16;

    private final List<EntityCache.Modifiable<?, ?>> invalidates = new ArrayList<>(DEFAULT_CACHE_SIZE);
    private final List<EntityCache.Modifiable.CacheWithCommitSupport<?, ?>> commits =
            new ArrayList<>(DEFAULT_CACHE_SIZE);

    protected <
            IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>
            > EntityCache.Modifiable.CacheWithCommitSupport<IdClass, EntityClass> register(
            EntityCache.Modifiable.CacheWithCommitSupport<IdClass, EntityClass> instance
    ) {
        invalidates.add(instance);
        commits.add(instance);
        return instance;
    }

    public void invalidateAll() {
        for (var object : invalidates) {
            object.invalidateAll();
        }
    }

    @Override
    public final void commit() {
        for (var object : commits) {
            object.commit();
        }
    }
}
