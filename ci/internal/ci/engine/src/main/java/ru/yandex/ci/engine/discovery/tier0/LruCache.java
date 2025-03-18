package ru.yandex.ci.engine.discovery.tier0;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.function.Function;

import javax.annotation.concurrent.NotThreadSafe;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Stopwatch;

import ru.yandex.lang.NonNullApi;

@NonNullApi
@NotThreadSafe
class LruCache<K, V> {

    private final LinkedHashMap<K, V> map;
    private final Function<K, V> loader;
    private final int maxSize;
    private final Stopwatch stopwatch = Stopwatch.createUnstarted();
    private long totalCount = 0;
    private long missCount = 0;

    LruCache(int maxSize, Function<K, V> loader) {
        map = new LinkedHashMap<>(maxSize + 1, 0.75f, true) {
            @Override
            protected boolean removeEldestEntry(Map.Entry<K, V> eldest) {
                return size() > maxSize;
            }
        };
        this.loader = loader;
        this.maxSize = maxSize;
    }

    V get(K key) {
        totalCount++;
        return map.computeIfAbsent(key, k -> {
            stopwatch.start();
            try {
                return loadAndIncrementStats(k);
            } finally {
                stopwatch.stop();
            }
        });
    }

    String cacheHitRate() {
        return String.format("%.2f", totalCount != 0 ? (totalCount - missCount) * 1.0 / totalCount : 0);
    }

    Stopwatch getStopwatch() {
        return stopwatch;
    }

    @VisibleForTesting
    LinkedHashMap<K, V> getMap() {
        return map;
    }

    long getTotalCount() {
        return totalCount;
    }

    long getMissCount() {
        return missCount;
    }

    int getMaxSize() {
        return maxSize;
    }

    private V loadAndIncrementStats(K key) {
        missCount++;
        return loader.apply(key);
    }
}
