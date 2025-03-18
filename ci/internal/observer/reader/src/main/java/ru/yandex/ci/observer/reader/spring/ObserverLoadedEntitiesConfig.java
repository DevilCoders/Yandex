package ru.yandex.ci.observer.reader.spring;

import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.reader.message.InternalPartitionGenerator;
import ru.yandex.ci.observer.reader.message.internal.InternalStreamStatistics;
import ru.yandex.ci.observer.reader.message.internal.ObserverEntitiesChecker;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;

@Configuration
@Import({
        ObserverReaderCoreConfig.class
})
public class ObserverLoadedEntitiesConfig {
    @Bean
    public InternalPartitionGenerator partitionGenerator(
            @Value("${observer.partitionGenerator.numberOfPartitions}") int numberOfPartitions
    ) {
        return new InternalPartitionGenerator(numberOfPartitions);
    }

    @Bean
    public LoadedPartitionEntitiesCache loadedEntitiesCache(
            InternalPartitionGenerator partitionGenerator,
            MeterRegistry meterRegistry,
            @Value("${observer.loadedEntitiesCache.expireAfter}") Duration expireAfter
    ) {
        return new LoadedPartitionEntitiesCache(partitionGenerator, expireAfter, meterRegistry);
    }

    @Bean
    public ObserverEntitiesChecker observerEntitiesChecker(
            InternalStreamStatistics observerInternalStreamStatistics,
            ObserverCache cache,
            LoadedPartitionEntitiesCache loadedEntitiesCache,
            CiObserverDb db,
            @Value("${observer.observerEntitiesChecker.retryDelay}") Duration retryDelay
    ) {
        return new ObserverEntitiesChecker(
                observerInternalStreamStatistics,
                cache,
                loadedEntitiesCache,
                db,
                retryDelay
        );
    }
}
