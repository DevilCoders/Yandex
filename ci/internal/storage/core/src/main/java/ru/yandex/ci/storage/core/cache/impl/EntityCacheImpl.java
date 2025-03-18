package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.collect.Lists;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.cache.CacheConstants;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.utils.MapUtils;

public abstract class EntityCacheImpl<IdClass extends Entity.Id<EntityClass>,
        EntityClass extends Entity<EntityClass>, CiDb extends TransactionSupportDefault>
        implements EntityCache<IdClass, EntityClass> {
    private static final Logger log = LoggerFactory.getLogger(EntityCacheImpl.class);

    protected final Class<EntityClass> entityClass;
    protected final Cache<IdClass, Optional<EntityClass>> cache;
    protected final CiDb db;
    protected final String cacheName;

    private final Function<IdClass, EntityClass> defaultProvider;
    private final Counter numberOfLoads;
    private final Counter numberOfGroupLoads;

    protected EntityCacheImpl(
            Class<EntityClass> entityClass, Cache<IdClass, Optional<EntityClass>> cache, CiDb db,
            MeterRegistry meterRegistry,
            String cacheName
    ) {
        this(entityClass, cache, db, meterRegistry, cacheName, null);
    }

    protected EntityCacheImpl(
            Class<EntityClass> entityClass, Cache<IdClass, Optional<EntityClass>> cache, CiDb db,
            MeterRegistry meterRegistry,
            String cacheName,
            @Nullable Function<IdClass, EntityClass> defaultProvider
    ) {
        this.entityClass = entityClass;
        this.cache = cache;
        this.db = db;
        this.cacheName = cacheName;
        this.defaultProvider = defaultProvider;

        this.numberOfGroupLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag("name", cacheName)
                .tag("action", CacheConstants.GROUP_LOAD)
                .register(meterRegistry);

        this.numberOfLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag("name", cacheName)
                .tag("action", CacheConstants.SINGLE_LOAD)
                .register(meterRegistry);
    }

    public static <IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>>
    Cache<IdClass, Optional<EntityClass>> createDefault(int capacity) {
        return createDefault(capacity, null, null);
    }

    public static <IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>>
    Cache<IdClass, Optional<EntityClass>> createDefault(
            int capacity, @Nullable Duration expireAfterAccess
    ) {
        return createDefault(capacity, null, expireAfterAccess);
    }

    public static <IdClass extends Entity.Id<EntityClass>, EntityClass extends Entity<EntityClass>>
    Cache<IdClass, Optional<EntityClass>> createDefault(
            int capacity, @Nullable Duration expireAfterWrite, @Nullable Duration expireAfterAccess
    ) {
        var builder = CacheBuilder.newBuilder();
        if (expireAfterWrite != null) {
            builder = builder.expireAfterWrite(expireAfterWrite);
        }

        if (expireAfterAccess != null) {
            builder = builder.expireAfterAccess(expireAfterAccess);
        }

        return builder
                .maximumSize(capacity)
                .recordStats()
                .build();
    }

    @Override
    public Optional<EntityClass> get(IdClass id) {
        try {
            return this.cache.get(id, () -> this.load(id));
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public List<EntityClass> get(Set<IdClass> ids) {
        if (ids.isEmpty()) {
            return List.of();
        }

        var idsToLoad = new ArrayList<IdClass>(ids.size());
        var entities = new ArrayList<EntityClass>(ids.size());

        for (var id : ids) {
            var cached = this.cache.getIfPresent(id);
            //noinspection OptionalAssignedToNull
            if (cached != null) {
                cached.ifPresent(entities::add);
            } else {
                idsToLoad.add(id);
            }
        }

        if (idsToLoad.isEmpty()) {
            return entities;
        }

        log.info("Loading {} of {}", idsToLoad.size(), entityClass.getSimpleName());
        this.numberOfGroupLoads.increment(idsToLoad.size());

        var loadedEntities = new ArrayList<EntityClass>(idsToLoad.size());
        for (var partition : Lists.partition(idsToLoad, YdbUtils.RESULT_ROW_LIMIT)) {
            loadedEntities.addAll(this.db.currentOrReadOnly(() -> this.getTable().find(new HashSet<>(partition))));
        }

        var loadedEntitiesMap = loadedEntities.stream().collect(Collectors.toMap(Entity::getId, Function.identity()));

        try {
            for (var id : idsToLoad) {
                cache.get(id, () -> Optional.ofNullable(loadedEntitiesMap.get(id))).ifPresent(entities::add);
            }
        } catch (ExecutionException e) {
            throw new RuntimeException("Failed to put to cache", e);
        }

        return entities;
    }

    @Override
    public Optional<EntityClass> getFresh(IdClass id) {
        var updated = this.load(id);
        this.cache.put(id, updated);
        return updated;
    }

    @Override
    public List<EntityClass> getFresh(Set<IdClass> ids) {
        return getFreshById(ids).values().stream().filter(Optional::isPresent).map(Optional::get).toList();
    }

    @Override
    @SuppressWarnings("unchecked")
    public Map<IdClass, Optional<EntityClass>> getFreshById(Set<IdClass> ids) {
        if (ids.isEmpty()) {
            return Map.of();
        }

        log.info("Loading fresh {} of {}", ids.size(), entityClass.getSimpleName());
        this.numberOfGroupLoads.increment(ids.size());

        var entities = ids.stream().collect(Collectors.toMap(
                        Function.identity(), x -> Optional.<EntityClass>empty(), MapUtils::forbidMerge, HashMap::new
                )
        );

        for (var partition : Lists.partition(new ArrayList<>(ids), YdbUtils.RESULT_ROW_LIMIT)) {
            this.db.currentOrReadOnly(() -> this.getTable().find(new HashSet<>(partition))).forEach(
                    entity -> entities.put((IdClass) entity.getId(), Optional.of(entity))
            );
        }

        cache.putAll(entities);

        return entities;
    }

    @Override
    public Optional<EntityClass> getIfPresent(IdClass id) {
        var value = this.cache.getIfPresent(id);
        //noinspection OptionalAssignedToNull
        return value == null ? Optional.empty() : value;
    }

    @Override
    public List<EntityClass> getIfPresent(Set<IdClass> ids) {
        return this.cache.getAllPresent(ids).values().stream()
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(Collectors.toList());
    }

    @Override
    public EntityClass getOrDefault(IdClass id) {
        if (defaultProvider == null) {
            throw new UnsupportedOperationException();
        }

        return get(id).orElse(defaultProvider.apply(id));
    }

    @Override
    public long size() {
        return cache.size();
    }

    protected Optional<EntityClass> load(IdClass id) {
        log.info("Loading {} {}", entityClass.getSimpleName(), id);
        this.numberOfLoads.increment();

        return this.db.currentOrReadOnly(() -> getTable().find(id));
    }

    protected abstract KikimrTableCi<EntityClass> getTable();
}
