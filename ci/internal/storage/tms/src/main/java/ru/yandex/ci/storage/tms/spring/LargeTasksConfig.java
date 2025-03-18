package ru.yandex.ci.storage.tms.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.CheckService;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.large.AutocheckTaskScheduler;
import ru.yandex.ci.storage.core.large.LargeAutostartBootstrapTask;
import ru.yandex.ci.storage.core.large.LargeAutostartMatchers;
import ru.yandex.ci.storage.core.large.LargeAutostartService;
import ru.yandex.ci.storage.core.large.LargeFlowStartService;
import ru.yandex.ci.storage.core.large.LargeFlowTask;
import ru.yandex.ci.storage.core.large.LargeLogbrokerTask;
import ru.yandex.ci.storage.core.large.LargeStartService;
import ru.yandex.ci.storage.core.large.MarkDiscoveredCommitTask;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.model.StorageEnvironment;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.storage.core.spring.StorageEnvironmentConfig;
import ru.yandex.ci.storage.core.spring.StorageEventProducerConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        BazingaCoreConfig.class,
        StorageTmsCacheConfig.class,
        StorageEventProducerConfig.class,
        ClientsConfig.class,
        StorageEnvironmentConfig.class
})
public class LargeTasksConfig {

    @Bean
    public RequirementsService requirementsService(
            BazingaTaskManager bazingaTaskManager, StorageEnvironment storageEnvironment
    ) {
        return new RequirementsService(bazingaTaskManager, storageEnvironment.getValue());
    }

    @Bean
    public CheckService checkService(RequirementsService requirementsService) {
        return new CheckService(requirementsService);
    }

    @Bean
    public LargeStartService largeStartService(
            CiStorageDb ciStorageDb,
            BazingaTaskManager bazingaTaskManager,
            RequirementsService requirementsService,
            CiClient ciClient,
            CheckService checkService,
            StorageEventsProducer storageEventsProducer,
            StorageCoreCache<?> storageTmsCache
    ) {
        return new LargeStartService(
                ciStorageDb,
                bazingaTaskManager,
                requirementsService,
                ciClient,
                checkService,
                storageEventsProducer,
                storageTmsCache
        );
    }

    @Bean
    public LargeAutostartMatchers largeAutostartMatchers() {
        return new LargeAutostartMatchers();
    }

    @Bean
    public AutocheckTaskScheduler autocheckTaskScheduler(
            LargeStartService largeStartService,
            RequirementsService requirementsService,
            StorageCoreCache<?> storageTmsCache
    ) {
        return new AutocheckTaskScheduler(
                largeStartService,
                requirementsService,
                storageTmsCache
        );
    }

    @Bean
    public LargeAutostartService largeAutostartService(
            CiStorageDb ciStorageDb,
            AutocheckTaskScheduler autocheckTaskScheduler,
            LargeAutostartMatchers largeAutostartMatchers,
            BazingaTaskManager bazingaTaskManager
    ) {
        return new LargeAutostartService(
                ciStorageDb, autocheckTaskScheduler, largeAutostartMatchers, bazingaTaskManager
        );
    }

    @Bean
    public LargeAutostartBootstrapTask largeAutostartBootstrapTask(LargeAutostartService largeAutostartService) {
        return new LargeAutostartBootstrapTask(largeAutostartService);
    }

    @Bean
    public LargeLogbrokerTask largeLogbrokerTask(LargeStartService largeStartService) {
        return new LargeLogbrokerTask(largeStartService);
    }

    @Bean
    public LargeFlowStartService largeFlowStartService(
            CiClient ciClient,
            StorageCoreCache<?> storageTmsCache,
            CiStorageDb db,
            BazingaTaskManager bazingaTaskManager
    ) {
        return new LargeFlowStartService(ciClient, storageTmsCache, db, bazingaTaskManager);
    }

    @Bean
    public LargeFlowTask largeFlowTask(LargeFlowStartService largeFlowStartService) {
        return new LargeFlowTask(largeFlowStartService);
    }

    @Bean
    public MarkDiscoveredCommitTask markDiscoveredCommitTask(CiClient ciClient, StorageCoreCache<?> storageTmsCache) {
        return new MarkDiscoveredCommitTask(ciClient, storageTmsCache);
    }
}
