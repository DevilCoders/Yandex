package ru.yandex.ci.storage.api.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.check.CheckComparer;
import ru.yandex.ci.storage.api.controllers.StorageApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontHistoryApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontTestsApiController;
import ru.yandex.ci.storage.api.controllers.StorageMaintenanceController;
import ru.yandex.ci.storage.api.controllers.StorageProxyApiController;
import ru.yandex.ci.storage.api.controllers.public_api.CommonPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.RawDataPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.SearchPublicApiController;
import ru.yandex.ci.storage.api.proxy.StorageMessageProxyWriterImpl;
import ru.yandex.ci.storage.api.search.LargeTestsSearch;
import ru.yandex.ci.storage.api.search.SearchService;
import ru.yandex.ci.storage.api.tests.HistoryService;
import ru.yandex.ci.storage.api.tests.TestsService;
import ru.yandex.ci.storage.api.util.CheckAttributesCollector;
import ru.yandex.ci.storage.api.util.TestRunCommandProvider;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.CheckService;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.large.AutocheckTasksFactory;
import ru.yandex.ci.storage.core.large.LargeStartService;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.model.StorageEnvironment;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.core.spring.BadgeEventsConfig;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.core.spring.ShardingConfig;
import ru.yandex.ci.storage.core.spring.StorageEnvironmentConfig;
import ru.yandex.ci.storage.core.spring.StorageEventProducerConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.core.test_metrics.MetricsClickhouseConfig;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        CommonConfig.class,
        LogbrokerConfig.class,
        ClientsConfig.class,
        StorageYdbConfig.class,
        StorageApiCacheConfig.class,
        StorageEventProducerConfig.class,
        BazingaCoreConfig.class,
        BadgeEventsConfig.class,
        ShardingConfig.class,
        MetricsClickhouseConfig.class,
        StorageEnvironmentConfig.class
})
public class StorageApiConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ApiCheckService apiCheckService(
            Clock clock,
            RequirementsService requirementsService,
            CiStorageDb db,
            ArcService arcService,
            StorageApiCache storageApiCache,
            StorageEventsProducer storageEventsProducer,
            StorageEnvironment storageEnvironment,
            @Value("${storage.apiCheckService.numberOfPartitions}") int numberOfPartitions,
            BazingaTaskManager bazingaTaskManager,
            ShardingSettings shardingSettings
    ) {
        return new ApiCheckService(
                clock,
                requirementsService,
                db,
                arcService,
                storageEnvironment.getValue(),
                storageApiCache,
                storageEventsProducer,
                numberOfPartitions,
                bazingaTaskManager,
                shardingSettings
        );
    }

    @Bean
    public StorageFrontTestsApiController storageFrontTestsApiController(TestsService testsService) {
        return new StorageFrontTestsApiController(testsService);
    }

    @Bean
    public FlowUrls urlService(@Value("${storage.urlService.urlPrefix}") String urlPrefix) {
        return new FlowUrls(urlPrefix);
    }

    @Bean
    public AutocheckTasksFactory largeTasksSearch(StorageApiCache storageApiCache) {
        return new AutocheckTasksFactory(storageApiCache);
    }

    @Bean
    public StorageApiController storageApiController(
            ApiCheckService checkService,
            StorageEventsProducer storageEventsProducer,
            StorageApiCache storageApiCache,
            BadgeEventsProducer badgeEventsProducer,
            AutocheckTasksFactory autocheckTasksFactory,
            CiStorageDb db,
            StorageProxyApiController storageProxyApiController
    ) {
        return new StorageApiController(
                checkService,
                storageApiCache,
                storageEventsProducer,
                badgeEventsProducer,
                autocheckTasksFactory,
                new CheckComparer(db),
                storageProxyApiController
        );
    }

    @Bean(destroyMethod = "shutdown")
    public HistoryService historySearchService(
            CiStorageDb db,
            @Value("${storage.historySearchService.pageSize}") int pageSize,
            @Value("${storage.historySearchService.wrappedPageSize}") int wrappedPageSize,
            @Value("${storage.historySearchService.numberOfThreads}") int numberOfThreads
    ) {
        return new HistoryService(db, pageSize, wrappedPageSize, numberOfThreads);
    }

    @Bean
    public TestsService testsService(CiStorageDb db) {
        return new TestsService(db);
    }


    @Bean
    public StorageFrontHistoryApiController storageFrontHistoryApiController(
            HistoryService historyService,
            TestMetricsService metricsService
    ) {
        return new StorageFrontHistoryApiController(historyService, metricsService);
    }

    @Bean
    public SearchService searchService(CiStorageDb db) {
        return new SearchService(db, 20, 20);
    }

    @Bean
    public RequirementsService requirementsService(
            BazingaTaskManager bazingaTaskManager,
            StorageEnvironment storageEnvironment
    ) {
        return new RequirementsService(bazingaTaskManager, storageEnvironment.getValue());
    }

    @Bean
    public LargeStartService largeStartService(
            CiStorageDb ciStorageDb,
            BazingaTaskManager bazingaTaskManager,
            RequirementsService requirementsService,
            CiClient ciClient,
            CheckService checkService,
            StorageEventsProducer storageEventsProducer,
            StorageCoreCache<?> storageApiCache
    ) {
        return new LargeStartService(
                ciStorageDb,
                bazingaTaskManager,
                requirementsService,
                ciClient,
                checkService,
                storageEventsProducer,
                storageApiCache
        );
    }

    @Bean
    public LargeTestsSearch largeTestsSearch(CiStorageDb db) {
        return new LargeTestsSearch(db);
    }

    @Bean
    public TestRunCommandProvider runCommandProvider(CiStorageDb db) {
        return new TestRunCommandProvider(db);
    }

    @Bean
    public CheckAttributesCollector checkAttributesCollector(FlowUrls flowUrls) {
        return new CheckAttributesCollector(flowUrls);
    }

    @Bean
    public StorageFrontApiController storageFrontApiController(
            ApiCheckService checkService,
            SearchService searchService,
            LargeTestsSearch largeTestsSearch,
            LargeStartService largeStartService,
            TestRunCommandProvider testRunCommandProvider,
            StorageCoreCache<?> storageApiCache,
            CheckAttributesCollector checkAttributesCollector,
            ArcService arcService
    ) {
        return new StorageFrontApiController(
                checkService,
                searchService,
                largeTestsSearch,
                largeStartService,
                storageApiCache,
                testRunCommandProvider,
                checkAttributesCollector,
                arcService
        );
    }

    @Bean
    public CommonPublicApiController commonPublicApiController(ApiCheckService apiCheckService) {
        return new CommonPublicApiController(apiCheckService);
    }

    @Bean
    public SearchPublicApiController searchPublicApiController(
            ApiCheckService apiCheckService
    ) {
        return new SearchPublicApiController(apiCheckService);
    }

    @Bean
    public RawDataPublicApiController rawDataPublicApiController(CiStorageDb db) {
        return new RawDataPublicApiController(db);
    }

    @Bean
    public StorageMaintenanceController storageMaintenanceController() {
        return new StorageMaintenanceController();
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public StorageProxyApiController storageProxyApiController(
            LogbrokerProxyBalancerHolder proxyHolder,
            @Qualifier("storage.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) {
        var writers = StorageMessageProxyWriterImpl.writersFor(proxyHolder, credentialsProvider, meterRegistry);
        return new StorageProxyApiController(writers);
    }
}
