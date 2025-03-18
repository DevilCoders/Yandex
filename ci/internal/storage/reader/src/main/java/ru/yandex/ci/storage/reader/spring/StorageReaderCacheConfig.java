package ru.yandex.ci.storage.reader.spring;

import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.cache.CheckMergeRequirementsCache;
import ru.yandex.ci.storage.core.cache.CheckTaskStatisticsCache;
import ru.yandex.ci.storage.core.cache.CheckTasksCache;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.ChunksGroupedCache;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.cache.SkippedChecksCache;
import ru.yandex.ci.storage.core.cache.impl.CheckMergeRequirementsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.CheckTaskStatisticsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.CheckTasksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ChecksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ChunksGroupedCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.IterationsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.SettingsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.SkippedChecksCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.reader.cache.ChunkAggregatesGroupedCache;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.cache.impl.ChunkAggregatesGroupedCacheImpl;
import ru.yandex.ci.storage.reader.cache.impl.ReaderCacheImpl;

@Configuration
@Import({
        CommonConfig.class,
        StorageYdbConfig.class
})
public class StorageReaderCacheConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public SettingsCache.WithModificationSupport readerSettingsCache(CiStorageDb ciStorageDb) {
        return new SettingsCacheImpl(ciStorageDb, meterRegistry);
    }

    @Bean
    public ChecksCache.WithModificationSupport readerChecksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerChecksCache.size}") int size
    ) {
        return new ChecksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckMergeRequirementsCache.WithModificationSupport readerMergeRequirementsCache(
            CiStorageDb ciStorageDb
    ) {
        return new CheckMergeRequirementsCacheImpl(ciStorageDb, 1000, meterRegistry);
    }

    @Bean
    public SkippedChecksCache.WithModificationSupport readerSkippedChecksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerSkippedChecksCache.size}") int size
    ) {
        return new SkippedChecksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public IterationsCache.WithModificationSupport readerIterationsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerIterationsCache.size}") int size
    ) {
        return new IterationsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckTasksCache.WithModificationSupport readerCheckTasksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerCheckTasksCache.size}") int size
    ) {
        return new CheckTasksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckTaskStatisticsCache.WithModificationSupport readerCheckTaskStatisticsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerCheckTaskStatisticsCache.size}") int size
    ) {
        return new CheckTaskStatisticsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public ChunkAggregatesGroupedCache.WithModificationSupport readerChunkAggregatesCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.readerChunkAggregatesCache.size}") int size
    ) {
        return new ChunkAggregatesGroupedCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public ChunksGroupedCache.WithModificationSupport readerChunksCache(CiStorageDb ciStorageDb) {
        return new ChunksGroupedCacheImpl(ciStorageDb, meterRegistry);
    }

    @Bean
    public ReaderCache storageReaderCache(
            CiStorageDb db,
            SettingsCache.WithModificationSupport readerSettingsCache,
            ChecksCache.WithModificationSupport readerChecksCache,
            CheckMergeRequirementsCache.WithModificationSupport readerMergeRequirementsCache,
            SkippedChecksCache.WithModificationSupport readerSkippedChecksCache,
            IterationsCache.WithModificationSupport readerIterationsCache,
            CheckTasksCache.WithModificationSupport readerCheckTasksCache,
            CheckTaskStatisticsCache.WithModificationSupport readerCheckTaskStatisticsCache,
            ChunkAggregatesGroupedCache.WithModificationSupport readerChunkAggregatesCache,
            ChunksGroupedCache.WithModificationSupport readerChunksCache,
            @Value("${storage.storageReaderCache.maxNumberOfWrites}") int maxNumberOfWrites
    ) {
        return new ReaderCacheImpl(
                db,
                List.of(
                        readerChecksCache,
                        readerMergeRequirementsCache,
                        readerIterationsCache,
                        readerCheckTasksCache,
                        readerSkippedChecksCache,
                        readerCheckTaskStatisticsCache,
                        readerChunkAggregatesCache,
                        readerChunksCache,
                        readerSettingsCache
                ),
                maxNumberOfWrites
        );
    }
}
