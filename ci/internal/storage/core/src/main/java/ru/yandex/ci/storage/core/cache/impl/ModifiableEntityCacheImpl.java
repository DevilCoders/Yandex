package ru.yandex.ci.storage.core.cache.impl;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import com.google.common.cache.Cache;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.exceptions.StorageException;

@SuppressWarnings("OptionalAssignedToNull")
public abstract class ModifiableEntityCacheImpl<
        IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>,
        CiDb extends TransactionSupportDefault
        > implements
        EntityCache.Modifiable<IdClass, EntityClass>,
        EntityCache.Modifiable.CacheWithCommitSupport<IdClass, EntityClass> {

    protected final int maxNumberOfWrites;

    protected final EntityCacheImpl<IdClass, EntityClass, CiDb> baseImpl;
    protected final Map<IdClass, Optional<EntityClass>> writeBuffer;
    protected final Set<IdClass> invalidatedIds;
    protected final Cache<IdClass, Optional<EntityClass>> cache;

    protected final boolean forbidWriteWithoutFreshRead;
    protected boolean invalidateAllRequested;

    protected ModifiableEntityCacheImpl(
            EntityCacheImpl<IdClass, EntityClass, CiDb> baseImpl,
            Cache<IdClass, Optional<EntityClass>> cache,
            int maxNumberOfWrites
    ) {
        this(baseImpl, cache, maxNumberOfWrites, true);
    }

    protected ModifiableEntityCacheImpl(
            EntityCacheImpl<IdClass, EntityClass, CiDb> baseImpl,
            Cache<IdClass, Optional<EntityClass>> cache,
            int maxNumberOfWrites,
            boolean forbidWriteWithoutFreshRead
    ) {
        this.baseImpl = baseImpl;
        this.cache = cache;
        this.maxNumberOfWrites = maxNumberOfWrites;
        this.writeBuffer = new ConcurrentHashMap<>();
        this.invalidatedIds = ConcurrentHashMap.newKeySet();
        this.forbidWriteWithoutFreshRead = forbidWriteWithoutFreshRead;
    }

    @Override
    public Optional<EntityClass> get(IdClass id) {
        var value = this.writeBuffer.get(id);
        if (value != null) {
            return value;
        }

        checkNotInvalidated(id);
        return this.baseImpl.get(id);
    }

    private void checkNotInvalidated(IdClass id) {
        if (invalidatedIds.contains(id)) {
            throw new UnsupportedOperationException("Read after invalidation not supported.");
        }
    }

    @Override
    public List<EntityClass> get(Set<IdClass> ids) {
        var inWriteCache = ids.stream().filter(this.writeBuffer::containsKey).collect(Collectors.toSet());
        if (inWriteCache.isEmpty()) {
            return this.baseImpl.get(ids);
        }

        var result = getExistingFromCache(inWriteCache);

        result.addAll(
                this.baseImpl.get(ids.stream().filter(id -> !inWriteCache.contains(id)).collect(Collectors.toSet()))
        );

        return result;
    }

    @Override
    public Optional<EntityClass> getIfPresent(IdClass id) {
        var value = this.writeBuffer.get(id);
        if (value != null) {
            return value;
        }

        checkNotInvalidated(id);
        return this.baseImpl.getIfPresent(id);
    }

    @Override
    public List<EntityClass> getIfPresent(Set<IdClass> ids) {
        var inWriteCache = ids.stream().filter(this.writeBuffer::containsKey).collect(Collectors.toSet());
        if (inWriteCache.isEmpty()) {
            return this.baseImpl.getIfPresent(ids);
        }

        var result = getExistingFromCache(inWriteCache);

        result.addAll(
                this.baseImpl.getIfPresent(
                        ids.stream().filter(id -> !inWriteCache.contains(id)).collect(Collectors.toSet())
                )
        );

        return result;
    }

    private List<EntityClass> getExistingFromCache(Set<IdClass> ids) {
        return ids.stream()
                .map(this.writeBuffer::get)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(Collectors.toList());
    }

    @Override
    public EntityClass getOrThrow(IdClass id) {
        var value = this.writeBuffer.get(id);
        if (value != null) {
            return value.orElseThrow(() -> new NoSuchElementException("Unable to find " + id));
        }

        checkNotInvalidated(id);
        return this.baseImpl.getOrThrow(id);
    }

    @Override
    public EntityClass getOrDefault(IdClass id) {
        var value = this.writeBuffer.get(id);
        if (value != null && value.isPresent()) {
            return value.get();
        }

        checkNotInvalidated(id);
        return this.baseImpl.getOrDefault(id);
    }

    @Override
    public void put(EntityClass entity) {
        Preconditions.checkNotNull(entity);
        if (this.writeBuffer.size() >= maxNumberOfWrites) {
            throw new StorageException("Number of writes exceeded: " + this.writeBuffer.size());
        }

        //noinspection unchecked
        var id = (IdClass) entity.getId();
        if (forbidWriteWithoutFreshRead && !this.writeBuffer.containsKey(id)) {
            throw new StaleModificationException(id.toString());
        }

        this.writeBuffer.put(id, Optional.of(entity));
    }

    @Override
    public Collection<EntityClass> getAffected() {
        return this.writeBuffer.values().stream().filter(Optional::isPresent).map(Optional::get).toList();
    }

    @Override
    public long size() {
        return this.baseImpl.size();
    }

    @Override
    public Optional<EntityClass> getFresh(IdClass id) {
        var value = this.writeBuffer.get(id);
        if (value != null) {
            return value;
        }

        var newValue = this.baseImpl.getFresh(id);
        this.writeBuffer.put(id, newValue);

        return newValue;
    }

    @Override
    public List<EntityClass> getFresh(Set<IdClass> ids) {
        return this.getFreshById(ids).values().stream().filter(Optional::isPresent).map(Optional::get).toList();
    }

    @Override
    public Map<IdClass, Optional<EntityClass>> getFreshById(Set<IdClass> ids) {
        if (ids.isEmpty()) {
            return Map.of();
        }

        var notInWriteCache = ids.stream()
                .filter(key -> !this.writeBuffer.containsKey(key))
                .collect(Collectors.toSet());

        var fromDb = this.baseImpl.getFreshById(notInWriteCache);
        this.writeBuffer.putAll(fromDb);

        return ids.stream().collect(Collectors.toMap(Function.identity(), writeBuffer::get));
    }

    @Override
    public void writeThrough(EntityClass entity) {
        put(entity);
        this.baseImpl.getTable().save(entity);
    }

    @Override
    public void writeThrough(Collection<EntityClass> entities) {
        entities.forEach(this::writeThrough);
    }

    @Override
    public void invalidate(IdClass id) {
        this.invalidatedIds.add(id);
    }

    @Override
    public void invalidateAll() {
        this.writeBuffer.clear();
        this.invalidateAllRequested = true;
    }

    @Override
    public void commit() {
        if (invalidateAllRequested) {
            this.cache.invalidateAll();
        }

        writeBuffer.forEach(this.cache::put);
        writeBuffer.clear();

        this.invalidatedIds.forEach(this.cache::invalidate);
    }
}
