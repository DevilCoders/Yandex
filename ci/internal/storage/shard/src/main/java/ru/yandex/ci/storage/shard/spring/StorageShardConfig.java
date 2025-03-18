package ru.yandex.ci.storage.shard.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
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
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriterImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.storage.shard.message.AggregatePoolProcessor;
import ru.yandex.ci.storage.shard.message.AggregateReporter;
import ru.yandex.ci.storage.shard.message.ChunkPoolProcessor;
import ru.yandex.ci.storage.shard.message.PostProcessorDeliveryService;
import ru.yandex.ci.storage.shard.message.ShardInReadProcessor;
import ru.yandex.ci.storage.shard.message.ShardInStreamConsumer;
import ru.yandex.ci.storage.shard.message.writer.PostProcessorWriter;
import ru.yandex.ci.storage.shard.message.writer.PostProcessorWriterImpl;
import ru.yandex.ci.storage.shard.task.AggregateProcessor;
import ru.yandex.ci.storage.shard.task.TaskDiffProcessor;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;

@Configuration
@Import({
        CommonConfig.class,
        StorageShardCacheConfig.class,
        StorageShardStatisticsConfig.class,
        LogbrokerConfig.class
})
public class StorageShardConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public TimeTraceService timeTraceService(Clock clock) {
        return new TimeTraceService(clock, collectorRegistry);
    }

    @Bean
    public PostProcessorDeliveryService postProcessorDeliveryService(
            ShardStatistics statistics,
            PostProcessorWriter postProcessorWriter,
            @Value("${storage.postProcessorDeliveryService.queueDrainLimit}") int queueDrainLimit,
            @Value("${storage.postProcessorDeliveryService.syncMode}") boolean syncMode
    ) {
        return new PostProcessorDeliveryService(
                statistics,
                postProcessorWriter,
                queueDrainLimit,
                syncMode
        );
    }

    @Bean
    public AggregatePoolProcessor aggregatePoolProcessor(
            CiStorageDb db,
            AggregateProcessor aggregateProcessor,
            ShardCache shardCache,
            AggregateReporter aggregateReporter,
            ShardOutMessageWriter writer,
            PostProcessorDeliveryService postProcessor,
            TimeTraceService timeTraceService,
            ShardStatistics statistics,
            @Value("${storage.aggregatePoolProcessor.numberOfQueues}") int numberOfQueues,
            @Value("${storage.aggregatePoolProcessor.queueDrainLimit}") int queueDrainLimit
    ) {
        return new AggregatePoolProcessor(
                db, aggregateProcessor, shardCache, aggregateReporter, writer,
                timeTraceService, statistics, postProcessor,
                numberOfQueues, queueDrainLimit
        );
    }

    @Bean
    public ChunkPoolProcessor chunkPoolProcessor(
            CiStorageDb db,
            ShardCache cache,
            TimeTraceService timeTraceService,
            ShardStatistics statistics,
            AggregatePoolProcessor aggregatePoolProcessor,
            @Value("${storage.chunkPoolProcessor.queueCapacity}") int queueCapacity,
            @Value("${storage.chunkPoolProcessor.queueDrainLimit}") int queueDrainLimit,
            @Value("${storage.chunkPoolProcessor.syncMode}") boolean syncMode

    ) {
        return new ChunkPoolProcessor(
                db,
                cache,
                timeTraceService,
                aggregatePoolProcessor,
                statistics,
                queueCapacity,
                queueDrainLimit,
                syncMode
        );
    }

    @Bean
    public ShardInReadProcessor shardReadProcessor(
            ChunkPoolProcessor chunkProcessor,
            ShardCache shardCache,
            ShardStatistics statistics
    ) {
        return new ShardInReadProcessor(
                chunkProcessor,
                shardCache,
                statistics
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public StreamListener shardStreamListener(
            ShardInReadProcessor readProcessor,
            ShardStatistics statistics,
            Clock clock,
            @Value("${storage.shardStreamListener.readQueues}") int readQueues,
            @Value("${storage.shardStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${storage.shardStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "shard_in",
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
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public ShardInStreamConsumer messageConsumer(
            LogbrokerConfiguration logbrokerConfiguration,
            StreamListener listener,
            SettingsCache settingsCache,
            @Value("${storage.messageConsumer.maxUncommittedReads}") int maxUncommittedReads,
            @Value("${storage.messageConsumer.onlyNewData}") boolean onlyNewData
    ) throws InterruptedException {
        return new ShardInStreamConsumer(
                logbrokerConfiguration,
                listener,
                settingsCache,
                onlyNewData,
                maxUncommittedReads
        );
    }

    @Bean
    public ShardOutMessageWriter notLocalShardOutMessageWriter(
            ShardCache cache,
            @Value("${storage.notLocalShardOutMessageWriter.numberOfPartitions}") int numberOfPartitions,
            LogbrokerWriterFactory notLocalShardOutLogbrokerWriterFactory
    ) {
        return new ShardOutMessageWriterImpl(
                cache.checks(), meterRegistry, numberOfPartitions, notLocalShardOutLogbrokerWriterFactory
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerWriterFactory notLocalShardOutLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.notLocalShardOutMessageWriter.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "shard_out",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

    @Bean
    public AggregateReporter aggregateReporter(
            ShardCache shardCache,
            ShardOutMessageWriter writer,
            ShardStatistics statistics,
            @Value("${storage.aggregateReporter.drainLimit}") int drainLimit,
            @Value("${storage.aggregateReporter.syncMode}") boolean syncMode

    ) {
        return new AggregateReporter(shardCache, writer, statistics, drainLimit, syncMode);
    }

    @Bean
    public TaskDiffProcessor taskDiffProcessor(ShardStatistics statistics) {
        return new TaskDiffProcessor(statistics);
    }

    @Bean
    public AggregateProcessor aggregateProcessor(TaskDiffProcessor taskDiffProcessor) {
        return new AggregateProcessor(taskDiffProcessor);
    }

    @Bean
    public PostProcessorWriter notLocalPostProcessorMessageWriter(
            @Value("${storage.notLocalPostProcessorMessageWriter.numberOfPartitions}") int numberOfPartitions,
            LogbrokerWriterFactory notLocalPostProcessorLogbrokerWriterFactory
    ) {
        return new PostProcessorWriterImpl(
                numberOfPartitions, meterRegistry, notLocalPostProcessorLogbrokerWriterFactory
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerWriterFactory notLocalPostProcessorLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.notLocalPostProcessorMessageWriter.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "post_processor_in",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }
}
