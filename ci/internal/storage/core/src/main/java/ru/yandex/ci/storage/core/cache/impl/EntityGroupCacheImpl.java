package ru.yandex.ci.storage.core.cache.impl;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.cache.EntityGroupCache;

public class EntityGroupCacheImpl<IdClass extends Entity.Id<EntityClass>,
        EntityClass extends Entity<EntityClass>> implements EntityGroupCache<IdClass, EntityClass> {
    private final Map<IdClass, EntityClass> cache = new ConcurrentHashMap<>();

    @SuppressWarnings("unchecked")
    public EntityGroupCacheImpl(List<EntityClass> entities) {
        entities.forEach(entity -> this.cache.put((IdClass) entity.getId(), entity));
    }

    @Override
    public Optional<EntityClass> get(IdClass id) {
        return Optional.ofNullable(cache.get(id));
    }

    public Collection<EntityClass> getAll() {
        return this.cache.values();
    }

    @SuppressWarnings("unchecked")
    @Override
    public void put(EntityClass entity) {
        this.cache.put((IdClass) entity.getId(), entity);
    }
}

