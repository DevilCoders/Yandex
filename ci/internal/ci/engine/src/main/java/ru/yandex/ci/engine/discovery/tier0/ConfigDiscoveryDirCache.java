package ru.yandex.ci.engine.discovery.tier0;

import java.time.Duration;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutionException;

import javax.annotation.Nonnull;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigDiscoveryDir;
import ru.yandex.ci.util.ExceptionUtils;

public class ConfigDiscoveryDirCache {

    private static final String CACHE_KEY = "cache";

    @Nonnull
    private final CiMainDb db;

    @Nonnull
    private final LoadingCache<String, PrefixPathToConfigMap> cache;

    public ConfigDiscoveryDirCache(
            CiMainDb db,
            Duration expireAfterWrite
    ) {
        this.db = db;
        cache = CacheBuilder.newBuilder()
                .expireAfterWrite(expireAfterWrite)
                .recordStats()
                .concurrencyLevel(1)
                .build(CacheLoader.from(this::load));
    }

    public Map<String, String> getPrefixPathToConfigMap() {
        try {
            return cache.get(CACHE_KEY);
        } catch (ExecutionException e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    private PrefixPathToConfigMap load(String key) {
        return db.currentOrReadOnly(() -> {
            int count = (int) db.configDiscoveryDirs().countAll();
            var prefixPathToConfigMap = new PrefixPathToConfigMap(count);

            db.configDiscoveryDirs()
                    .readTable(ReadTableParams.<ConfigDiscoveryDir.Id>builder()
                            .ordered()
                            .build()
                    )
                    .filter(it -> it.getDeleted() == null || !it.getDeleted())
                    .map(ConfigDiscoveryDir::getId)
                    .forEach(it -> prefixPathToConfigMap.put(
                            it.getPathPrefix(), it.getConfigPath()
                    ));

            return prefixPathToConfigMap;
        });
    }

    @VisibleForTesting
    public void flushCaches() {
        cache.invalidateAll();
    }

    private static class PrefixPathToConfigMap extends HashMap<String, String> {
        private PrefixPathToConfigMap(int initialCapacity) {
            super(initialCapacity);
        }
    }

}
