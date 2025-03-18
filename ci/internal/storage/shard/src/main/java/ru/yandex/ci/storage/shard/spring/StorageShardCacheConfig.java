package ru.yandex.ci.storage.shard.spring;

import java.time.Duration;
import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.storage.core.cache.CheckMergeRequirementsCache;
import ru.yandex.ci.storage.core.cache.CheckTasksCache;
import ru.yandex.ci.storage.core.cache.CheckTextSearchCache;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.ChunksGroupedCache;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.cache.impl.CheckMergeRequirementsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.CheckTasksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.CheckTextSearchCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ChecksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ChunksGroupedCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.IterationsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.SettingsCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.spring.AYamlerClientConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.shard.cache.ChunkAggregatesCache;
import ru.yandex.ci.storage.shard.cache.MuteStatusCache;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.storage.shard.cache.StrongModeCache;
import ru.yandex.ci.storage.shard.cache.TestDiffsCache;
import ru.yandex.ci.storage.shard.cache.impl.ChunkAggregatesCacheImpl;
import ru.yandex.ci.storage.shard.cache.impl.MuteStatusCacheImpl;
import ru.yandex.ci.storage.shard.cache.impl.ShardCacheImpl;
import ru.yandex.ci.storage.shard.cache.impl.StrongModeCacheImpl;
import ru.yandex.ci.storage.shard.cache.impl.TestDiffsCacheImpl;

@Configuration
@Import({
        StorageYdbConfig.class,
        AYamlerClientConfig.class
})
public class StorageShardCacheConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public SettingsCache.WithModificationSupport shardSettingsCache(CiStorageDb ciStorageDb) {
        return new SettingsCacheImpl(ciStorageDb, meterRegistry);
    }

    @Bean
    public MuteStatusCacheImpl shardMuteStatusCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardMuteStatusCache.syncPeriod}") Duration syncPeriod
    ) {
        return new MuteStatusCacheImpl(ciStorageDb, meterRegistry, syncPeriod);
    }

    @Bean
    public ChecksCache.WithModificationSupport shardChecksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardChecksCache.size}") int size
    ) {
        return new ChecksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckMergeRequirementsCache.WithModificationSupport shardMergeRequirementsCache(
            CiStorageDb ciStorageDb
    ) {
        return new CheckMergeRequirementsCacheImpl(ciStorageDb, 1000, meterRegistry);
    }

    @Bean
    public ChunkAggregatesCache.WithModificationSupport shardChunkAggregates(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardChunkAggregates.size}") int size
    ) {
        return new ChunkAggregatesCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public TestDiffsCache.WithModificationSupport shardTestDiffsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardTestDiffsCache.size}") int size
    ) {
        return new TestDiffsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckTasksCache.WithModificationSupport shardCheckTasksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardCheckTasksCache.size}") int size
    ) {
        return new CheckTasksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public ChunksGroupedCache.WithModificationSupport shardChunksCache(CiStorageDb ciStorageDb) {
        return new ChunksGroupedCacheImpl(ciStorageDb, meterRegistry);
    }

    @Bean
    public StrongModeCache shardStrongModeCache(
            AYamlerClient aYamlerClient,
            @Value("${storage.shardStrongModeCache.expiresAfterAccess}") Duration expiresAfterAccess,
            @Value("${storage.shardStrongModeCache.size}") long size
    ) {
        return new StrongModeCacheImpl(expiresAfterAccess, size, aYamlerClient, meterRegistry);
    }

    @Bean
    public IterationsCache.WithModificationSupport shardIterationsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.shardIterationsCache.size}") int size
    ) {
        return new IterationsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public CheckTextSearchCache checkTextSearchCache() {
        return new CheckTextSearchCacheImpl(meterRegistry);
    }

    @Bean
    public ShardCache shardStorageCache(
            CiStorageDb db,
            SettingsCache.WithModificationSupport shardSettingsCache,
            MuteStatusCache shardMuteStatusCache,
            ChecksCache.WithModificationSupport shardChecksCache,
            CheckMergeRequirementsCache.WithModificationSupport shardMergeRequirementsCache,
            ChunkAggregatesCache.WithModificationSupport shardChunkAggregates,
            TestDiffsCache.WithModificationSupport shardTestDiffsCache,
            CheckTasksCache.WithModificationSupport shardCheckTasksCache,
            ChunksGroupedCache.WithModificationSupport shardChunksCache,
            StrongModeCache shardStrongModeCache,
            IterationsCache.WithModificationSupport shardIterationsCache,
            CheckTextSearchCache checkTextSearchCache,
            @Value("${storage.shardStorageCache.maxNumberOfWrites}") int maxNumberOfWrites
    ) {
        return new ShardCacheImpl(
                db,
                List.of(
                        shardSettingsCache,
                        shardChecksCache,
                        shardMergeRequirementsCache,
                        shardChunkAggregates,
                        shardTestDiffsCache,
                        shardCheckTasksCache,
                        shardChunksCache,
                        shardIterationsCache
                ),
                List.of(shardStrongModeCache, checkTextSearchCache, shardMuteStatusCache),
                maxNumberOfWrites
        );
    }


}
