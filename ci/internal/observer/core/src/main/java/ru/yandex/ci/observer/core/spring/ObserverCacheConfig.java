package ru.yandex.ci.observer.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.observer.core.cache.CheckIterationsGroupingCache;
import ru.yandex.ci.observer.core.cache.CheckIterationsGroupingCacheImpl;
import ru.yandex.ci.observer.core.cache.CheckTaskPartitionTraceCache;
import ru.yandex.ci.observer.core.cache.CheckTaskPartitionTraceCacheImpl;
import ru.yandex.ci.observer.core.cache.CheckTasksGroupingCache;
import ru.yandex.ci.observer.core.cache.CheckTasksGroupingCacheImpl;
import ru.yandex.ci.observer.core.cache.ChecksCache;
import ru.yandex.ci.observer.core.cache.ChecksCacheImpl;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.cache.ObserverCacheImpl;
import ru.yandex.ci.observer.core.cache.ObserverSettingsCache;
import ru.yandex.ci.observer.core.cache.ObserverSettingsCacheImpl;
import ru.yandex.ci.observer.core.db.CiObserverDb;

@Configuration
@Import({
        CommonConfig.class,
        ObserverYdbConfig.class
})
public class ObserverCacheConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ObserverSettingsCache.WithModificationSupport settingsCache(CiObserverDb ciDb) {
        return new ObserverSettingsCacheImpl(ciDb, meterRegistry);
    }

    @Bean
    public ChecksCache.WithModificationSupport observerChecksCache(
            CiObserverDb ciDb,
            @Value("${observer.observerChecksCache.size}") int size
    ) {
        return new ChecksCacheImpl(ciDb, size, meterRegistry);
    }

    @Bean
    public CheckIterationsGroupingCache.WithModificationSupport iterationsGroupingCache(
            CiObserverDb ciDb,
            @Value("${observer.iterationsGroupingCache.size}") int size
    ) {
        return new CheckIterationsGroupingCacheImpl(size, meterRegistry, ciDb);
    }

    @Bean
    public CheckTasksGroupingCache.WithModificationSupport tasksGroupingCache(
            CiObserverDb ciDb,
            @Value("${observer.tasksGroupingCache.size}") int size
    ) {
        return new CheckTasksGroupingCacheImpl(size, meterRegistry, ciDb);
    }

    @Bean
    public CheckTaskPartitionTraceCache.WithModificationSupport tracesCache(
            CiObserverDb ciDb,
            @Value("${observer.tracesCache.size}") int size
    ) {
        return new CheckTaskPartitionTraceCacheImpl(ciDb, size, meterRegistry);
    }

    @Bean
    public ObserverCache observerCache(
            CiObserverDb db,
            ObserverSettingsCache.WithModificationSupport settings,
            ChecksCache.WithModificationSupport observerChecksCache,
            CheckIterationsGroupingCache.WithModificationSupport iterationsGroupingCache,
            CheckTasksGroupingCache.WithModificationSupport tasksGroupingCache,
            CheckTaskPartitionTraceCache.WithModificationSupport tracesCache,
            @Value("${observer.observerCache.maxNumberOfWrites}") int maxNumberOfWrites
    ) {
        return new ObserverCacheImpl(
                db,
                settings,
                observerChecksCache,
                iterationsGroupingCache,
                tasksGroupingCache,
                tracesCache,
                maxNumberOfWrites
        );
    }
}
