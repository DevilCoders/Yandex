package ru.yandex.ci.storage.post_processor.spring;

import java.time.Clock;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProperties;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.logbroker.LogbrokerTopics;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.logbroker.LogbrokerStreamConsumer;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.logbroker.StreamListenerCommitWatchdog;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.core.test_metrics.MetricsClickhouseConfig;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.core.ydb.SequenceService;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.storage.post_processor.logbroker.PostProcessorInReadProcessor;
import ru.yandex.ci.storage.post_processor.processing.HistoryProcessor;
import ru.yandex.ci.storage.post_processor.processing.MessageProcessor;
import ru.yandex.ci.storage.post_processor.processing.MessageProcessorPool;
import ru.yandex.ci.storage.post_processor.processing.MetricsProcessor;
import ru.yandex.ci.storage.post_processor.processing.MetricsProcessorPool;
import ru.yandex.ci.storage.post_processor.processing.MuteSettings;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;

@Configuration
@Import({
        CommonConfig.class,
        PostProcessorCacheConfig.class,
        LogbrokerConfig.class,
        MetricsClickhouseConfig.class
})
public class StoragePostProcessorConfig {

    @Bean
    public PostProcessorInReadProcessor postProcessorInReadProcessor(
            MessageProcessorPool messageProcessor,
            PostProcessorStatistics statistics,
            PostProcessorCache cache
    ) {
        return new PostProcessorInReadProcessor(messageProcessor, statistics, cache);
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamListener postProcessorStreamListener(
            PostProcessorInReadProcessor readProcessor,
            PostProcessorStatistics statistics,
            Clock clock,
            @Value("${storage.postProcessorStreamListener.readQueues}") int readQueues,
            @Value("${storage.postProcessorStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${storage.postProcessorStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "post_processor_in",
                readProcessor,
                statistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamConsumer postProcessorMessageConsumer(
            LogbrokerConfiguration logbrokerConfiguration,
            StreamListener listener,
            @Value("${storage.postProcessorMessageConsumer.maxUncommittedReads}") int maxUncommittedReads,
            @Value("${storage.postProcessorMessageConsumer.onlyNewData}") boolean onlyNewData
    ) throws InterruptedException {
        return new LogbrokerStreamConsumer(
                logbrokerConfiguration,
                listener,
                onlyNewData,
                maxUncommittedReads
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerConfiguration logbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${storage.logbrokerConfiguration.topicsString}") String topicsString,
            LogbrokerProperties logbrokerProperties,
            @Qualifier("storage.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) {
        return new LogbrokerConfiguration(
                proxyBalancerHolder,
                LogbrokerTopics.parse(topicsString),
                logbrokerProperties,
                credentialsProvider
        );
    }

    @Bean
    public MuteSettings muteSettings() {
        return new MuteSettings(10, 1000);
    }

    @Bean
    public HistoryProcessor historyProcessor(
            CiStorageDb db,
            PostProcessorStatistics statistics,
            PostProcessorCache cache,
            TimeTraceService timeTraceService,
            FragmentationSettings fragmentationSettings
    ) {
        return new HistoryProcessor(db, statistics, cache.testHistory(), timeTraceService, fragmentationSettings);
    }

    @Bean
    public MetricsProcessor metricsProcessor(
            TestMetricsService testMetricsService, PostProcessorStatistics statistics
    ) {
        return new MetricsProcessor(testMetricsService, statistics);
    }

    @Bean
    public MetricsProcessorPool metricsProcessorPool(
            PostProcessorStatistics statistics,
            MetricsProcessor metricsProcessor,
            @Value("${storage.metricsProcessorPool.queueCapacity}") int queueCapacity,
            @Value("${storage.metricsProcessorPool.queueDrainLimit}") int queueDrainLimit,
            @Value("${storage.metricsProcessorPool.numberOfQueues}") int numberOfQueues
    ) {
        return new MetricsProcessorPool(
                statistics, metricsProcessor, queueCapacity, queueDrainLimit, numberOfQueues
        );
    }

    @Bean
    public MessageProcessor messageProcessor(
            Clock clock,
            CiStorageDb db,
            PostProcessorCache cache,
            PostProcessorStatistics statistics,
            MuteSettings muteSettings,
            TimeTraceService timeTraceService,
            HistoryProcessor historyProcessor,
            MetricsProcessorPool metricsProcessorPool
    ) {
        return new MessageProcessor(
                clock, db, cache, timeTraceService, statistics, muteSettings, historyProcessor, metricsProcessorPool
        );
    }

    @Bean
    public MessageProcessorPool messageProcessorPool(
            MessageProcessor messageProcessor,
            PostProcessorStatistics statistics,
            @Value("${storage.messageProcessorPool.queueCapacity}") int queueCapacity,
            @Value("${storage.messageProcessorPool.queueDrainLimit}") int queueDrainLimit,
            @Value("${storage.messageProcessorPool.numberOfQueues}") int numberOfQueues
    ) {
        return new MessageProcessorPool(statistics, messageProcessor, queueCapacity, queueDrainLimit, numberOfQueues);
    }

    @Bean
    public SequenceService sequenceService(CiStorageDb db) {
        return new SequenceService(db);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public StreamListenerCommitWatchdog streamListenerCommitWatchdog(
            Clock clock, LogbrokerStreamListener postProcessorStreamListener
    ) {
        return new StreamListenerCommitWatchdog(clock, postProcessorStreamListener, 120);
    }
}
