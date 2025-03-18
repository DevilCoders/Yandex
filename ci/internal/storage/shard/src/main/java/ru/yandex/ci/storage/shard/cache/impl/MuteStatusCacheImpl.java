package ru.yandex.ci.storage.shard.cache.impl;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.collect.Lists;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.Entity;

import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.cache.CacheConstants;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.shard.cache.MuteStatusCache;

@Slf4j
public class MuteStatusCacheImpl implements MuteStatusCache {

    private final CiStorageDb db;
    private final Cache<TestStatusEntity.Id, Boolean> cache;
    private final AtomicReference<Instant> lastInvalidate;
    private final Duration invalidatePeriod;

    private final Counter numberOfGroupLoads;
    private final Counter numberOfLoads;

    public MuteStatusCacheImpl(CiStorageDb db, MeterRegistry meterRegistry, Duration invalidatePeriod) {
        this.db = db;
        this.invalidatePeriod = invalidatePeriod;

        this.cache = CacheBuilder.newBuilder()
                .expireAfterAccess(Duration.ofHours(2)) // unused keys
                .recordStats()
                .build();

        this.lastInvalidate = new AtomicReference<>(Instant.now());

        var cacheName = "mute-status";
        GuavaCacheMetrics.monitor(meterRegistry, cache, cacheName);

        this.numberOfGroupLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag("name", cacheName)
                .tag("action", CacheConstants.GROUP_LOAD)
                .register(meterRegistry);

        this.numberOfLoads = Counter.builder(CacheConstants.METRIC_NAME)
                .tag("name", cacheName)
                .tag("action", CacheConstants.SINGLE_LOAD)
                .register(meterRegistry);
    }

    @Override
    public void syncMuteActionsIfNeeded() {
        var now = Instant.now();
        var lastInvalidateValue = this.lastInvalidate.get();
        if (lastInvalidateValue.isBefore(now.minus(invalidatePeriod))) {
            if (lastInvalidate.compareAndSet(lastInvalidateValue, now)) {
                this.db.currentOrReadOnly(() -> syncMuteActions(lastInvalidateValue, 100000));
            }
        }
    }

    private void syncMuteActions(Instant lastInvalidateValue, int limit) {
        var updated = this.db.testMutes().getLastActions(lastInvalidateValue, limit);
        log.info("Synced mute actions for {} tests", updated.size());
        updated.forEach(action -> this.cache.put(action.getId().getTestId(), action.isMuted()));
    }

    @Override
    public Map<TestStatusEntity.Id, Boolean> get(Set<TestStatusEntity.Id> ids) {
        if (ids.isEmpty()) {
            return Map.of();
        }

        var idsToLoad = new ArrayList<TestStatusEntity.Id>(ids.size());
        var entities = new HashMap<TestStatusEntity.Id, Boolean>(ids.size());

        for (var id : ids) {
            var cached = this.cache.getIfPresent(id);
            if (cached != null) {
                entities.put(id, cached);
            } else {
                idsToLoad.add(id);
            }
        }

        if (idsToLoad.isEmpty()) {
            return entities;
        }

        log.info("Loading {} of test mute status", idsToLoad.size());

        var loadedEntities = new ArrayList<TestStatusEntity>(idsToLoad.size());
        for (var partition : Lists.partition(idsToLoad, YdbUtils.RESULT_ROW_LIMIT)) {
            loadedEntities.addAll(this.db.currentOrReadOnly(() -> db.tests().find(new HashSet<>(partition))));
        }

        var loadedEntitiesMap = loadedEntities.stream().collect(Collectors.toMap(Entity::getId, Function.identity()));

        try {
            for (var id : idsToLoad) {
                entities.put(
                        id,
                        cache.get(
                                id, () -> Optional.ofNullable(loadedEntitiesMap.get(id))
                                        .map(TestStatusEntity::isMuted).orElse(false)
                        )
                );
            }
        } catch (ExecutionException e) {
            throw new RuntimeException("Failed to put to cache", e);
        }

        this.numberOfGroupLoads.increment();

        return entities;
    }

    @Override
    public boolean get(TestStatusEntity.Id id) {
        try {
            return this.cache.get(id, () -> load(id));
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private Boolean load(TestStatusEntity.Id id) {
        log.info("Loading single test mute status: {}", id);
        this.numberOfLoads.increment();
        return this.db.currentOrReadOnly(() -> this.db.tests().find(id).map(TestStatusEntity::isMuted).orElse(false));
    }

    @Override
    public void invalidateAll() {
        cache.invalidateAll();
    }
}
