package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;

import ru.yandex.ci.storage.core.cache.CheckTextSearchCache;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;

@Value
public class CheckTextSearchCacheImpl implements CheckTextSearchCache {
    Cache<Long, Set<CheckTextSearchEntity.Id>> cache;

    public CheckTextSearchCacheImpl(MeterRegistry meterRegistry) {
        this.cache = CacheBuilder.newBuilder()
                .expireAfterAccess(Duration.ofMinutes(10))
                .recordStats()
                .build();

        GuavaCacheMetrics.monitor(meterRegistry, cache, "check-text-search");
    }

    @Override
    public boolean contains(CheckTextSearchEntity.Id id) {
        return get(id.getCheckId()).contains(id);
    }

    @Override
    public void put(List<CheckTextSearchEntity> values) {
        var byCheckId = values.stream()
                .map(CheckTextSearchEntity::getId)
                .collect(Collectors.groupingBy(CheckTextSearchEntity.Id::getCheckId));

        for (var group : byCheckId.entrySet()) {
            get(group.getKey()).addAll(group.getValue());
        }
    }

    private Set<CheckTextSearchEntity.Id> get(long checkId) {
        try {
            return this.cache.get(checkId, ConcurrentHashMap::newKeySet);
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void invalidateAll() {
        this.cache.invalidateAll();
    }
}
