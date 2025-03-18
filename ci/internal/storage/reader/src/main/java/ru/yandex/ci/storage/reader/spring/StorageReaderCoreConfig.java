package ru.yandex.ci.storage.reader.spring;

import java.time.Clock;
import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.model.StorageEnvironment;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.core.spring.BadgeEventsConfig;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.core.spring.ShardingConfig;
import ru.yandex.ci.storage.core.spring.StorageEnvironmentConfig;
import ru.yandex.ci.storage.core.spring.StorageEventProducerConfig;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckAnalysisService;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.check.listeners.ArcanumCheckEventsListener;
import ru.yandex.ci.storage.reader.check.listeners.BadgeEventsListener;
import ru.yandex.ci.storage.reader.check.listeners.ObserverCheckEventsListener;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.storage.reader.other.MetricAggregationService;
import ru.yandex.ci.storage.reader.registration.RegistrationProcessorEmptyImpl;
import ru.yandex.ci.storage.reader.registration.RegistrationProcessorImpl;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        CommonConfig.class,
        BazingaCoreConfig.class,
        LogbrokerConfig.class,
        TvmClientConfig.class,
        StorageReaderCacheConfig.class,
        ShardingConfig.class,
        StorageEventProducerConfig.class,
        ShardInStreamConfig.class,
        BadgeEventsConfig.class,
        CheckAnalysisConfig.class,
        StorageEnvironmentConfig.class
})
public class StorageReaderCoreConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public ReaderStatistics readerStatistics() {
        return new ReaderStatistics(
                new MainStreamStatistics(meterRegistry, collectorRegistry),
                new ShardOutStreamStatistics(meterRegistry, collectorRegistry),
                new EventsStreamStatistics(meterRegistry, collectorRegistry),
                meterRegistry
        );
    }

    @Bean
    public TimeTraceService timeTraceService() {
        return new TimeTraceService(Clock.systemUTC(), collectorRegistry);
    }

    @Bean
    public MetricAggregationService metricAggregationService(ReaderStatistics statistics) {
        return new MetricAggregationService(statistics);
    }

    @Bean
    public ReaderCheckService readerCheckService(
            RequirementsService requirementsService,
            ReaderCache cache,
            ReaderStatistics readerStatistics,
            CiStorageDb db,
            BadgeEventsProducer badgeEventsProducer,
            MetricAggregationService metricAggregationService
    ) {
        return new ReaderCheckService(
                requirementsService, cache,
                readerStatistics, db, badgeEventsProducer, metricAggregationService
        );
    }

    @Bean
    public CheckFinalizationService checkFinalizationService(
            ReaderCache cache, ReaderStatistics readerStatistics, CiStorageDb db,
            List<CheckEventsListener> eventListeners, ShardInMessageWriter shardMessageWriter,
            BazingaTaskManager bazingaTaskManager, CheckAnalysisService analysisService,
            ReaderCheckService readerCheckService
    ) {
        return new CheckFinalizationService(
                cache, readerStatistics, db, eventListeners, shardMessageWriter, bazingaTaskManager, analysisService,
                readerCheckService
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_STABLE_PROFILE)
    public RegistrationProcessor notStableRegistrationProcessor(
            CiStorageDb db,
            ReaderCache readerCache,
            ReaderCheckService checkService,
            ShardingSettings shardingSettings,
            @Value("${storage.notStableRegistrationProcessor.numberOfPartitions}") int numberOfPartitions
    ) {
        return new RegistrationProcessorImpl(db, readerCache, checkService, numberOfPartitions, shardingSettings);
    }

    @Bean
    @Profile(value = CiProfile.STABLE_PROFILE)
    public RegistrationProcessor stableRegistrationProcessor() {
        return new RegistrationProcessorEmptyImpl();
    }


    @Bean
    public RequirementsService requirementsService(
            BazingaTaskManager bazingaTaskManager,
            StorageEnvironment storageEnvironment
    ) {
        return new RequirementsService(bazingaTaskManager, storageEnvironment.getValue());
    }

    @Bean
    public ArcanumCheckEventsListener arcanumCheckStatusReporter(
            RequirementsService requirementsService
    ) {
        return new ArcanumCheckEventsListener(requirementsService);
    }

    @Bean
    public ObserverCheckEventsListener observerCheckEventsReporter(
            StorageEventsProducer eventsProducer
    ) {
        return new ObserverCheckEventsListener(eventsProducer);
    }

    @Bean
    public BadgeEventsListener badgeEventsListener(
            BadgeEventsProducer badgeEventsProducer
    ) {
        return new BadgeEventsListener(badgeEventsProducer);
    }
}
