package ru.yandex.ci.storage.core.cache;

import java.util.Optional;

import javax.annotation.concurrent.ThreadSafe;

import yandex.cloud.repository.db.Entity;

@ThreadSafe
public interface EntityGroupCache<IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>> {
    /**
     * Gets from cache, will load from db if not present.
     *
     * @param id entity id.
     * @return entity.
     */
    Optional<EntityClass> get(IdClass id);

    /**
     * Updates value in cache.
     *
     * @param entity entity.
     */
    void put(EntityClass entity);
}
