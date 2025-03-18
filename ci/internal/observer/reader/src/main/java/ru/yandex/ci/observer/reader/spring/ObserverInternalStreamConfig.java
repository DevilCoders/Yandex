package ru.yandex.ci.observer.reader.spring;

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
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.reader.check.ObserverInternalCheckService;
import ru.yandex.ci.observer.reader.message.InternalPartitionGenerator;
import ru.yandex.ci.observer.reader.message.ObserverStreamConsumer;
import ru.yandex.ci.observer.reader.message.internal.InternalStreamStatistics;
import ru.yandex.ci.observer.reader.message.internal.ObserverEntitiesChecker;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamMessageProcessor;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamQueuedReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriterImpl;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;

@Configuration
@Import({
        ObserverLoadedEntitiesConfig.class,
        LogbrokerConfig.class
})
public class ObserverInternalStreamConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public ObserverInternalStreamMessageProcessor observerInternalStreamMessageProcessor(
            ObserverInternalCheckService checkService,
            ObserverEntitiesChecker checker
    ) {
        return new ObserverInternalStreamMessageProcessor(checkService, checker);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ObserverInternalStreamQueuedReadProcessor observerInternalStreamQueuedReadProcessor(
            InternalStreamStatistics statistics,
            ObserverInternalStreamMessageProcessor messageProcessor,
            @Value("${observer.observerInternalStreamQueuedReadProcessor.drainLimit}") int drainLimit,
            @Value("${observer.observerInternalStreamQueuedReadProcessor.queueMaxNumber}") int queueMaxNumber
    ) {
        return new ObserverInternalStreamQueuedReadProcessor(
                statistics, messageProcessor, drainLimit, queueMaxNumber, false
        );
    }

    @Bean
    public ObserverInternalStreamReadProcessor observerInternalStreamReadProcessor(
            ObserverCache cache,
            ObserverEntitiesChecker entitiesChecker,
            LoadedPartitionEntitiesCache loadedEntitiesCache,
            InternalStreamStatistics observerInternalStreamStatistics,
            ObserverInternalStreamQueuedReadProcessor queuedReadProcessor
    ) {
        return new ObserverInternalStreamReadProcessor(
                cache, entitiesChecker, loadedEntitiesCache, observerInternalStreamStatistics, queuedReadProcessor
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamListener observerInternalStreamListener(
            ObserverInternalStreamReadProcessor readProcessor,
            InternalStreamStatistics statistics,
            Clock clock,
            @Value("${observer.observerInternalStreamListener.readQueues}") int readQueues,
            @Value("${observer.observerInternalStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${observer.observerInternalStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "internal",
                readProcessor,
                statistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerConfiguration observerInternalStreamLogbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${observer.observerInternalStreamLogbrokerConfiguration.topic}") String topic,
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
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ObserverStreamConsumer observerInternalStreamConsumer(
            LogbrokerConfiguration observerInternalStreamLogbrokerConfiguration,
            LogbrokerStreamListener observerInternalStreamListener,
            ObserverCache cache,
            @Value("${observer.observerInternalStreamConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${observer.observerInternalStreamConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ObserverStreamConsumer(
                observerInternalStreamLogbrokerConfiguration,
                observerInternalStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }

    @Bean
    public ObserverInternalStreamWriter observerInternalStreamWriter(
            InternalPartitionGenerator partitionGenerator,
            @Value("${observer.observerInternalStreamWriter.numberOfPartitions}") int numberOfPartitions,
            LogbrokerWriterFactory observerInternalLogbrokerWriterFactory
    ) {
        return new ObserverInternalStreamWriterImpl(
                meterRegistry, numberOfPartitions, partitionGenerator, observerInternalLogbrokerWriterFactory
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerWriterFactory observerInternalLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            @Value("${observer.observerInternalStreamWriter.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "internal",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }
}
