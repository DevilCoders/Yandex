package ru.yandex.ci.storage.shard.cache.impl;

import java.time.Duration;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.client.ayamler.StrongModeRequest;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultYamlInfo;
import ru.yandex.ci.storage.shard.cache.StrongModeCache;

public class StrongModeCacheImpl implements StrongModeCache {

    private final Cache<StrongModeRequest, TestResultYamlInfo> cache;
    private final AYamlerClient aYamlerClient;

    public StrongModeCacheImpl(
            Duration expireAfterAccess,
            long size,
            AYamlerClient aYamlerClient,
            MeterRegistry meterRegistry
    ) {
        this.aYamlerClient = aYamlerClient;
        this.cache = CacheBuilder.newBuilder()
                .expireAfterAccess(expireAfterAccess)
                .maximumSize(size)
                .recordStats()
                .build();

        GuavaCacheMetrics.monitor(meterRegistry, cache, "ayamler");
    }

    @Override
    public CompletableFuture<Map<StrongModeRequest, TestResultYamlInfo>> getStrongMode(
            Set<StrongModeRequest> requests
    ) {
        var presentValues = cache.getAllPresent(requests);

        var notPresentValues = requests.stream()
                .filter(it -> !presentValues.containsKey(it))
                .collect(Collectors.toSet());

        if (notPresentValues.isEmpty()) {
            return CompletableFuture.completedFuture(presentValues);
        }

        return aYamlerClient.getStrongMode(notPresentValues)
                .thenApply(response -> {
                    var resultMap = new HashMap<>(presentValues);
                    for (var strongModeResponse : response.getStrongModeList()) {
                        var key = new StrongModeRequest(
                                strongModeResponse.getPath(),
                                strongModeResponse.getRevision(),
                                strongModeResponse.getLogin()
                        );

                        var strongMode = protoToStrongMode(strongModeResponse, key);
                        resultMap.put(key, strongMode);
                        cache.put(key, strongMode);
                    }
                    return resultMap;
                });
    }

    @Override
    public void invalidateAll() {
        cache.invalidateAll();
    }

    private static TestResultYamlInfo protoToStrongMode(Ayamler.StrongMode strongMode, StrongModeRequest request) {
        return TestResultYamlInfo.of(
                switch (strongMode.getStatus()) {
                    case ON -> TestResultYamlInfo.StrongModeStatus.ON;
                    case OFF -> TestResultYamlInfo.StrongModeStatus.OFF;
                    case NOT_FOUND -> throw new RuntimeException(
                            "Path doesn't exist in arc: %s, strongModeStatus=%s".formatted(request, strongMode)
                    );
                    case UNRECOGNIZED, UNKNOWN -> throw new RuntimeException(
                            "For %s got strongModeStatus=%s".formatted(request, strongMode)
                    );
                },
                strongMode.getAyaml().getPath(),
                strongMode.getAyaml().getService(),
                strongMode.getIsOwner()
        );
    }
}
