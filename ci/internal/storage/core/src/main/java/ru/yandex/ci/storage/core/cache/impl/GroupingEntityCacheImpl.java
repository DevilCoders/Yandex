package ru.yandex.ci.storage.core.cache.impl;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;

import com.google.common.base.Preconditions;
import com.google.common.cache.Cache;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.CacheConstants;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.GroupingEntityCache;
import ru.yandex.ci.storage.core.exceptions.StorageException;

public abstract class GroupingEntityCacheImpl<AggregateId, IdClass extends Entity.Id<EntityClass>,
        EntityClass extends Entity<EntityClass>> implements GroupingEntityCache<AggregateId, IdClass, EntityClass> {
    private static final Logger log = LoggerFactory.getLogger(GroupingEntityCacheImpl.class);

    protected final Cache<AggregateId, EntityGroupCacheImpl<IdClass, EntityClass>> cache;
    private final Counter numberOfEmptyLoads;
    private final Counter numberOfLoads;

    protected GroupingEntityCacheImpl(
            Cache<AggregateId, EntityGroupCacheImpl<IdClass, EntityClass>> cache,
            MeterRegistry meterRegistry,
            String cacheName
    ) {
        this.cache = cache;

        this.numberOfLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag(CacheConstants.NAME, cacheName)
                .tag(CacheConstants.ACTION, CacheConstants.GROUP_LOAD)
                .register(meterRegistry);

        this.numberOfEmptyLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag(CacheConstants.NAME, cacheName)
                .tag(CacheConstants.ACTION, CacheConstants.EMPTY_LOAD)
                .register(meterRegistry);
    }

    @Override
    public Optional<EntityClass> get(IdClass id) {
        return get(getAggregateId(id)).get(id);
    }

    @Override
    public EntityGroupCacheImpl<IdClass, EntityClass> get(AggregateId aggregateId) {
        try {
            return cache.get(aggregateId, () -> {
                log.debug("Loading group for {}", aggregateId);
                this.numberOfLoads.increment();
                return load(aggregateId);
            });
        } catch (ExecutionException e) {
            throw new RuntimeException("Failed to load", e);
        }
    }

    @Override
    public Optional<EntityClass> getFresh(IdClass id) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<EntityClass> getFresh(Set<IdClass> ids) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<IdClass, Optional<EntityClass>> getFreshById(Set<IdClass> ids) {
        throw new UnsupportedOperationException();
    }

    @Override
    public EntityGroupCacheImpl<IdClass, EntityClass> getForEmpty(AggregateId aggregateId) {
        try {
            return cache.get(aggregateId, () -> {
                log.debug("Loading empty group for {}", aggregateId);
                this.numberOfEmptyLoads.increment();
                return loadForEmpty(aggregateId);
            });
        } catch (ExecutionException e) {
            throw new RuntimeException("Failed to load", e);
        }
    }

    @Override
    public List<EntityClass> get(Set<IdClass> ids) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Optional<EntityClass> getIfPresent(IdClass id) {
        var groupCache = this.cache.getIfPresent(getAggregateId(id));
        //noinspection OptionalAssignedToNull
        return groupCache == null ? Optional.empty() : groupCache.get(id);
    }

    @Override
    public List<EntityClass> getIfPresent(Set<IdClass> ids) {
        throw new UnsupportedOperationException();
    }

    @Override
    public EntityClass getOrDefault(IdClass id) {
        throw new UnsupportedOperationException();
    }

    @Override
    public long size() {
        return this.cache.size();
    }

    protected abstract AggregateId getAggregateId(IdClass id);

    protected abstract EntityGroupCacheImpl<IdClass, EntityClass> load(AggregateId aggregateId);

    protected abstract EntityGroupCacheImpl<IdClass, EntityClass> loadForEmpty(AggregateId aggregateId);

    protected abstract KikimrTableCi<EntityClass> getTable();

    public static class Modifiable<
            AggregateId,
            IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>
            >
            implements
            GroupingEntityCache.Modifiable<AggregateId, IdClass, EntityClass>,
            EntityCache.Modifiable.CacheWithCommitSupport<IdClass, EntityClass> {
        protected final Map<IdClass, EntityClass> writeBuffer = new ConcurrentHashMap<>();

        protected final GroupingEntityCacheImpl<AggregateId, IdClass, EntityClass> baseImpl;
        protected final Cache<AggregateId, EntityGroupCacheImpl<IdClass, EntityClass>> cache;
        protected final int maxNumberOfWrites;
        protected boolean shouldInvalidateAll;

        protected final Set<AggregateId> invalidatedIds;

        public Modifiable(
                GroupingEntityCacheImpl<AggregateId, IdClass, EntityClass> baseImpl,
                Cache<AggregateId, EntityGroupCacheImpl<IdClass, EntityClass>> cache,
                int maxNumberOfWrites
        ) {
            this.baseImpl = baseImpl;
            this.cache = cache;
            this.maxNumberOfWrites = maxNumberOfWrites;
            this.invalidatedIds = ConcurrentHashMap.newKeySet();
        }

        @Override
        public Optional<EntityClass> get(IdClass id) {
            var value = this.writeBuffer.get(id);
            if (value != null) {
                return Optional.of(value);
            }

            return this.baseImpl.get(id);
        }

        public EntityGroupCacheImpl<IdClass, EntityClass> get(AggregateId aggregateId) {
            return this.baseImpl.get(aggregateId);
        }

        public EntityGroupCacheImpl<IdClass, EntityClass> getForEmpty(AggregateId aggregateId) {
            return this.baseImpl.getForEmpty(aggregateId);
        }

        @Override
        public List<EntityClass> get(Set<IdClass> ids) {
            throw new UnsupportedOperationException();
        }

        @Override
        public Optional<EntityClass> getIfPresent(IdClass id) {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<EntityClass> getIfPresent(Set<IdClass> ids) {
            throw new UnsupportedOperationException();
        }

        @Override
        public EntityClass getOrThrow(IdClass id) {
            return this.baseImpl.getOrThrow(id);
        }

        @Override
        public EntityClass getOrDefault(IdClass id) {
            throw new UnsupportedOperationException();
        }

        @Override
        public long size() {
            return this.baseImpl.size();
        }

        @Override
        public Optional<EntityClass> getFresh(IdClass id) {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<EntityClass> getFresh(Set<IdClass> ids) {
            throw new UnsupportedOperationException();
        }

        @Override
        public Map<IdClass, Optional<EntityClass>> getFreshById(Set<IdClass> ids) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void put(EntityClass entity) {
            Preconditions.checkNotNull(entity);
            if (this.writeBuffer.size() >= maxNumberOfWrites) {
                throw new StorageException("Number of writes exceeded: " + this.writeBuffer.size());
            }

            //noinspection unchecked
            this.writeBuffer.put((IdClass) entity.getId(), entity);
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
        public void invalidate(AggregateId aggregateId) {
            this.invalidatedIds.add(aggregateId);
        }

        @Override
        public void invalidate(IdClass id) {
            throw new UnsupportedOperationException();
        }

        @Override
        public Collection<EntityClass> getAffected() {
            return this.writeBuffer.values();
        }

        @Override
        public void invalidateAll() {
            this.writeBuffer.clear();
            this.shouldInvalidateAll = true;
        }

        @Override
        public void commit() {
            if (shouldInvalidateAll) {
                this.cache.invalidateAll();
            }

            writeBuffer.forEach((key, value) -> this.baseImpl.get(this.baseImpl.getAggregateId(key)).put(value));
            writeBuffer.clear();

            this.invalidatedIds.forEach(this.cache::invalidate);
        }
    }
}
