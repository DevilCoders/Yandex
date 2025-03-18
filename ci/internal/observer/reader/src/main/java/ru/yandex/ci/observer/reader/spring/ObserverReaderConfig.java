package ru.yandex.ci.observer.reader.spring;

import java.time.Clock;

import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.ObserverStreamConsumer;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.message.main.ObserverMainStreamMessageProcessor;
import ru.yandex.ci.observer.reader.message.main.ObserverMainStreamReadProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.spring.stream.MainStreamCoreConfig;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;


@Configuration
@Import({
        MainStreamCoreConfig.class,
        ObserverReaderEventsConfig.class,
        ObserverReaderCoreConfig.class
})
public class ObserverReaderConfig {

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public TimeTraceService timeTraceService() {
        return new TimeTraceService(Clock.systemUTC(), collectorRegistry);
    }

    @Bean
    public ObserverMainStreamMessageProcessor observerMainStreamMessageProcessor(
            ObserverCheckService observerCheckService,
            MainStreamStatistics statistics,
            ObserverInternalStreamWriter internalStreamWriter
    ) {
        return new ObserverMainStreamMessageProcessor(observerCheckService, statistics, internalStreamWriter);
    }

    @Bean
    public MainStreamReadProcessor observerMainStreamReadProcessor(
            TimeTraceService timeTraceService,
            ObserverMainStreamMessageProcessor observerTracesProcessor,
            MainStreamStatistics statistics
    ) {
        return new ObserverMainStreamReadProcessor(
                timeTraceService,
                observerTracesProcessor,
                statistics
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public StreamListener observerStreamListener(
            MainStreamReadProcessor observerMainStreamReadProcessor,
            MainStreamStatistics statistics,
            Clock clock,
            @Value("${observer.observerStreamListener.readQueues}") int readQueues,
            @Value("${observer.observerStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${observer.observerStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "main",
                observerMainStreamReadProcessor,
                statistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ObserverStreamConsumer messageConsumer(
            LogbrokerConfiguration mainStreamLogbrokerConfiguration,
            StreamListener observerStreamListener,
            ObserverCache cache,
            @Value("${observer.messageConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${observer.messageConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ObserverStreamConsumer(
                mainStreamLogbrokerConfiguration,
                observerStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }
}
