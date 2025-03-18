package ru.yandex.ci.storage.tms.spring;

import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.cache.CheckMergeRequirementsCache;
import ru.yandex.ci.storage.core.cache.CheckTasksCache;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.DefaultStorageCache;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.LargeTasksCache;
import ru.yandex.ci.storage.core.cache.impl.CheckMergeRequirementsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.CheckTasksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ChecksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.DefaultStorageCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.IterationsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.LargeTasksCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;

@Configuration
@Import({
        CommonConfig.class,
        StorageYdbConfig.class
})
public class StorageTmsCacheConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ChecksCache.WithModificationSupport tmsChecksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.tmsChecksCache.size}") int size
    ) {
        return new ChecksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckMergeRequirementsCache.WithModificationSupport tmsMergeRequirementsCache(
            CiStorageDb ciStorageDb
    ) {
        return new CheckMergeRequirementsCacheImpl(ciStorageDb, 1000, meterRegistry);
    }

    @Bean
    public IterationsCache.WithModificationSupport tmsIterationsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.tmsIterationsCache.size}") int size
    ) {
        return new IterationsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckTasksCache.WithModificationSupport tmsCheckTasksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.tmsCheckTasksCache.size}") int size
    ) {
        return new CheckTasksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public LargeTasksCache.WithModificationSupport tmsLargeTasksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.tmsLargeTasksCache.size}") int size
    ) {
        return new LargeTasksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public DefaultStorageCache storageTmsCache(
            CiStorageDb db,
            ChecksCache.WithModificationSupport tmsChecksCache,
            CheckMergeRequirementsCache.WithModificationSupport tmsMergeRequirementsCache,
            IterationsCache.WithModificationSupport tmsIterationsCache,
            CheckTasksCache.WithModificationSupport tmsCheckTasksCache,
            LargeTasksCache.WithModificationSupport tmsLargeTasksCache,
            @Value("${storage.storageTmsCache.maxNumberOfWrites}") int maxNumberOfWrites
    ) {
        return new DefaultStorageCacheImpl(
                db,
                List.of(
                        tmsChecksCache,
                        tmsMergeRequirementsCache,
                        tmsIterationsCache,
                        tmsCheckTasksCache,
                        tmsLargeTasksCache
                ),
                maxNumberOfWrites
        );
    }
}
