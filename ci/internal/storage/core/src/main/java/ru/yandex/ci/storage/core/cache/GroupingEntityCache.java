package ru.yandex.ci.storage.core.cache;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.storage.core.cache.impl.EntityGroupCacheImpl;

public interface GroupingEntityCache<
        AggregateId,
        IdClass extends Entity.Id<EntityClass>,
        EntityClass extends Entity<EntityClass>
        >
        extends EntityCache<IdClass, EntityClass> {
    EntityGroupCacheImpl<IdClass, EntityClass> get(AggregateId aggregateId);

    EntityGroupCacheImpl<IdClass, EntityClass> getForEmpty(AggregateId aggregateId);

    interface Modifiable<
            AggregateId,
            IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>
            >
            extends EntityCache.Modifiable<IdClass, EntityClass> {

        void invalidate(AggregateId aggregateId);
    }
}
