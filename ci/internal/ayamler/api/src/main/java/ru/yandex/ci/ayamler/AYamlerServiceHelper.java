package ru.yandex.ci.ayamler;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import com.google.common.cache.Cache;

import ru.yandex.ci.core.arc.ArcRevision;

public final class AYamlerServiceHelper {

    private AYamlerServiceHelper() {
    }

    public static <TKey extends PathGetter, TValue> Map<TKey, CompletableFuture<TValue>> loadFuturesFromCache(
            Cache<TKey, CompletableFuture<TValue>> cache,
            CacheLoader<TKey, CompletableFuture<TValue>> cacheLoader,
            Set<TKey> cacheKeys
    ) {
        return loadFromCache(
                cache, cacheLoader,
                (CompletableFuture<TValue> presentCacheValue) ->
                        (presentCacheValue.isDone() && !presentCacheValue.isCompletedExceptionally())
                                || !presentCacheValue.isDone(),
                cacheKeys
        );
    }

    public static <TKey extends PathGetter, TValue> Map<TKey, TValue> loadFromCache(
            Cache<TKey, TValue> cache,
            CacheLoader<TKey, TValue> cacheLoader,
            Set<TKey> cacheKeys
    ) {
        return loadFromCache(cache, cacheLoader, presentValue -> true, cacheKeys);
    }

    private static <TKey extends PathGetter, TValue> Map<TKey, TValue> loadFromCache(
            Cache<TKey, TValue> cache,
            CacheLoader<TKey, TValue> cacheLoader,
            PresentCacheValueIsValid<TValue> presentCacheValueIsValid,
            Set<TKey> cacheKeys
    ) {
        var presentCacheValues = new HashMap<TKey, TValue>();
        var notPresentKeys = new HashSet<TKey>();

        for (var cacheKey : cacheKeys) {
            var cacheValue = cache.getIfPresent(cacheKey);
            if (cacheValue != null && presentCacheValueIsValid.test(cacheValue)) {
                presentCacheValues.put(cacheKey, cacheValue);
            } else {
                notPresentKeys.add(cacheKey);
            }
        }

        if (!notPresentKeys.isEmpty()) {
            var keyValueMap = cacheLoader.load(notPresentKeys);
            cache.putAll(keyValueMap);
            presentCacheValues.putAll(keyValueMap);
        }

        return presentCacheValues;
    }

    public static ArcRevision correctRevisionFormat(ArcRevision revision) {
        return ArcRevision.parse(revision.getCommitId());
    }

    @FunctionalInterface
    public interface CacheLoader<TKey, TValue> {
        Map<TKey, TValue> load(Set<TKey> paths);
    }

    @FunctionalInterface
    public interface PresentCacheValueIsValid<T> {
        boolean test(T cacheValue);
    }

}
