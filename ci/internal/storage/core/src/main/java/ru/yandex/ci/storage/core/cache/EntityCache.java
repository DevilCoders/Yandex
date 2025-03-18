package ru.yandex.ci.storage.core.cache;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.concurrent.ThreadSafe;

import yandex.cloud.repository.db.Entity;

@ThreadSafe
public interface EntityCache<IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>> {
    /**
     * Gets from cache, will load from db if not present.
     *
     * @param id entity id.
     * @return entity.
     */
    Optional<EntityClass> get(IdClass id);

    /**
     * Gets from cache, will load from db all missing values.
     *
     * @param ids set of ids.
     * @return Value for each id.
     */
    List<EntityClass> get(Set<IdClass> ids);

    /**
     * Gets from cache, will load from db all missing values, if not present will use default.
     *
     * @param ids set of ids.
     * @return Value for each id.
     */
    default List<EntityClass> getOrDefault(Set<IdClass> ids) {
        var existingValues = new ArrayList<>(get(ids));
        var existingIds = existingValues.stream().map(Entity::getId).collect(Collectors.toSet());

        existingValues.addAll(
                ids.stream().filter(Predicate.not(existingIds::contains)).map(this::getOrDefault).toList()
        );

        return existingValues;
    }

    /**
     * Gets from cache, returns Optional.Empty for missing value.
     *
     * @param id entity id.
     * @return Optional.Empty if not found, overwise Optional.of(entity).
     */
    Optional<EntityClass> getIfPresent(IdClass id);

    /**
     * Gets from cache.
     *
     * @param ids set of ids.
     * @return Value for each id if present.
     */
    List<EntityClass> getIfPresent(Set<IdClass> ids);

    /**
     * Gets from cache, will load from db if not present. If missing will throw an exception.
     *
     * @param id entity id.
     * @return entity.
     */
    default EntityClass getOrThrow(IdClass id) {
        return get(id).orElseThrow(() -> new NoSuchElementException("Unable to find " + id));
    }

    /**
     * Gets from cache, will load from db if not present, if not found will use default
     *
     * @param id entity id.
     * @return entity.
     */
    EntityClass getOrDefault(IdClass id);

    /**
     * Gets cache size.
     *
     * @return Size of the cache.
     */
    long size();

    /**
     * Gets from write buffer or from db.
     *
     * @param id entity id.
     * @return entity.
     */
    Optional<EntityClass> getFresh(IdClass id);

    /**
     * Gets from db
     *
     * @param ids set of ids.
     * @return Value for each existing id.
     */
    List<EntityClass> getFresh(Set<IdClass> ids);

    /**
     * Gets from db
     *
     * @param ids set of ids.
     * @return Map id to value.
     */
    Map<IdClass, Optional<EntityClass>> getFreshById(Set<IdClass> ids);

    /**
     * Gets from write buffer or from db.
     *
     * @param id entity id.
     * @return entity.
     */
    default EntityClass getFreshOrThrow(IdClass id) {
        return getFresh(id).orElseThrow(() -> new NoSuchElementException("Unable to find " + id));
    }

    interface ModificationSupport<EntityIdClass extends Entity.Id<EntityClass>,
            EntityClass extends Entity<EntityClass>> {
        Modifiable.CacheWithCommitSupport<EntityIdClass, EntityClass> toModifiable(int maxNumberOfWrites);
    }

    @ThreadSafe
    interface Modifiable<EntityIdClass extends Entity.Id<EntityClass>,
            EntityClass extends Entity<EntityClass>> extends EntityCache<EntityIdClass, EntityClass> {
        /**
         * Updates value in cache.
         *
         * @param entity entity.
         */
        void put(EntityClass entity);

        /**
         * Updates value in cache.
         *
         * @param entities entities.
         */
        default void put(Collection<EntityClass> entities) {
            entities.forEach(this::put);
        }

        /**
         * Updates value in cache and db.
         *
         * @param entities entities.
         */
        void writeThrough(EntityClass entities);

        /**
         * Updates value in cache and db.
         *
         * @param entities entities.
         */
        void writeThrough(Collection<EntityClass> entities);

        /**
         * Removes from cache.
         *
         * @param id entity id.
         */
        void invalidate(EntityIdClass id);

        /**
         * Gets affected entities (written or loaded fresh from db).
         *
         * @return affected entities.
         */
        Collection<EntityClass> getAffected();

        /**
         * Removes all values from cache.
         */
        void invalidateAll();

        interface CacheWithCommitSupport<EntityIdClass extends Entity.Id<EntityClass>,
                EntityClass extends Entity<EntityClass>> extends Modifiable<EntityIdClass, EntityClass>, CommitSupport {
        }

        interface CommitSupport {
            void commit();
        }
    }
}
