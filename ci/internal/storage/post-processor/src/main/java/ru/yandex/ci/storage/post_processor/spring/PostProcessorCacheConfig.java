package ru.yandex.ci.storage.post_processor.spring;

import java.time.Clock;
import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.cache.TestStatisticsCache;
import ru.yandex.ci.storage.core.cache.TestsCache;
import ru.yandex.ci.storage.core.cache.impl.ChecksCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.IterationsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.SettingsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.TestStatisticsCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.TestsCacheImpl;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.storage.post_processor.cache.impl.PostProcessorCacheImpl;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCache;
import ru.yandex.ci.storage.post_processor.history.TestHistoryCacheImpl;

@Configuration
@Import({
        CommonConfig.class,
        StorageYdbConfig.class,
        StoragePostProcessorCoreConfig.class
})
public class PostProcessorCacheConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public SettingsCache.WithModificationSupport postProcessorSettingsCache(CiStorageDb ciStorageDb) {
        return new SettingsCacheImpl(ciStorageDb, meterRegistry);
    }

    @Bean
    public TestsCache.WithModificationSupport postProcessorTestsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.postProcessorTestsCache.size}") int size
    ) {
        return new TestsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public TestStatisticsCache.WithModificationSupport postProcessorTestStatisticsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.postProcessorTestStatisticsCache.size}") int size
    ) {
        return new TestStatisticsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public ChecksCache.WithModificationSupport postProcessorChecksCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.postProcessorChecksCache.size}") int size
    ) {
        return new ChecksCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public IterationsCache.WithModificationSupport postProcessorIterationsCache(
            CiStorageDb ciStorageDb,
            @Value("${storage.postProcessorIterationsCache.size}") int size
    ) {
        return new IterationsCacheImpl(ciStorageDb, size, meterRegistry);
    }

    @Bean
    public TestHistoryCache testHistoryCache(
            CiStorageDb db,
            PostProcessorStatistics statistics,
            Clock clock,
            FragmentationSettings fragmentationSettings,
            @Value("${storage.testHistoryCache.numberOfPartitions}") int numberOfPartitions
    ) {
        return new TestHistoryCacheImpl(db, statistics, numberOfPartitions, clock, fragmentationSettings);
    }

    @Bean
    public PostProcessorCache postProcessorCache(
            CiStorageDb db,
            ChecksCache.WithModificationSupport postProcessorChecksCache,
            IterationsCache.WithModificationSupport postProcessorIterationsCache,
            SettingsCache.WithModificationSupport postProcessorSettingsCache,
            TestsCache.WithModificationSupport postProcessorTestsCache,
            TestHistoryCache testHistoryCache,
            TestStatisticsCache.WithModificationSupport testStatisticsCache,
            @Value("${storage.postProcessorCache.maxNumberOfWrites}") int maxNumberOfWrites
    ) {
        return new PostProcessorCacheImpl(
                db,
                List.of(
                        postProcessorChecksCache,
                        postProcessorIterationsCache,
                        postProcessorSettingsCache,
                        postProcessorTestsCache,
                        testStatisticsCache
                ),
                List.of(testHistoryCache),
                maxNumberOfWrites
        );
    }
}
