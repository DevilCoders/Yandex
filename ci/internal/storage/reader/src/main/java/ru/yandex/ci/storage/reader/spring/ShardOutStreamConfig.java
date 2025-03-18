package ru.yandex.ci.storage.reader.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
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
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriterImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.message.ReaderStreamConsumer;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.shard.ChunkMessageProcessor;
import ru.yandex.ci.storage.reader.message.shard.ForwardingMessageProcessor;
import ru.yandex.ci.storage.reader.message.shard.ShardOutReadProcessor;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;


@Configuration
@Import(StorageReaderCoreConfig.class)
public class ShardOutStreamConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ForwardingMessageProcessor forwardingMessageProcessor(
            ReaderCheckService checkService,
            CheckFinalizationService checkFinalizationService,
            ReaderCache readerCache,
            ReaderStatistics readerStatistics
    ) {
        return new ForwardingMessageProcessor(
                checkService, checkFinalizationService, readerCache, readerStatistics
        );
    }

    @Bean
    public ChunkMessageProcessor chunkMessageProcessor(
            ReaderCheckService checkService,
            CheckFinalizationService checkFinalizationService,
            ReaderCache readerCache,
            ReaderStatistics statistics
    ) {
        return new ChunkMessageProcessor(checkService, checkFinalizationService, readerCache, statistics);
    }

    @Bean
    public ShardOutReadProcessor shardOutReadProcessor(
            ReaderCache readerCache,
            TimeTraceService timeTraceService,
            ForwardingMessageProcessor forwardingMessageProcessor,
            ReaderStatistics statistics,
            ChunkMessageProcessor chunkMessageProcessor
    ) {
        return new ShardOutReadProcessor(
                forwardingMessageProcessor, readerCache, statistics, chunkMessageProcessor, timeTraceService
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamListener shardOutStreamListener(
            ShardOutReadProcessor messageProcessor,
            ReaderStatistics statistics,
            Clock clock,
            @Value("${storage.shardOutStreamListener.readQueues}") int readQueues,
            @Value("${storage.shardOutStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${storage.shardOutStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "shard_out",
                messageProcessor,
                statistics.getShard(),
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerConfiguration shardOutLogbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${storage.shardOutLogbrokerConfiguration.topic}") String topic,
            LogbrokerProperties logbrokerProperties,
            @Qualifier("storage.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) {
        return new LogbrokerConfiguration(
                proxyBalancerHolder,
                LogbrokerTopics.parse(topic),
                logbrokerProperties,
                credentialsProvider
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public ReaderStreamConsumer shardOutStreamConsumer(
            LogbrokerConfiguration shardOutLogbrokerConfiguration,
            LogbrokerStreamListener shardOutStreamListener,
            ReaderCache cache,
            @Value("${storage.shardOutStreamConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${storage.shardOutStreamConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ReaderStreamConsumer(
                shardOutLogbrokerConfiguration,
                shardOutStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ShardOutMessageWriter notLocalShardOutMessageWriter(
            ReaderCache cache,
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

}
