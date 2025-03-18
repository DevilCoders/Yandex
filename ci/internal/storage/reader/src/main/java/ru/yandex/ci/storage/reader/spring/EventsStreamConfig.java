package ru.yandex.ci.storage.reader.spring;

import java.time.Clock;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;
import ru.yandex.ci.storage.core.spring.stream.EventsStreamCoreConfig;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.ReaderStreamConsumer;
import ru.yandex.ci.storage.reader.message.events.EventsMessageProcessor;
import ru.yandex.ci.storage.reader.message.events.EventsPoolProcessor;
import ru.yandex.ci.storage.reader.message.events.EventsStreamReadProcessor;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;

@Configuration
@Import({
        StorageReaderCoreConfig.class,
        EventsStreamCoreConfig.class,
        ShardInStreamConfig.class,
        ShardOutStreamConfig.class
})
public class EventsStreamConfig {
    @Bean
    public EventsMessageProcessor eventsMessageProcessor(
            ReaderCache readerCache,
            ShardOutMessageWriter writer,
            RegistrationProcessor registrationProcessor,
            ReaderStatistics statistics
    ) {
        return new EventsMessageProcessor(
                readerCache, writer, registrationProcessor, statistics
        );
    }

    @Bean
    public EventsPoolProcessor eventsPoolProcessor(
            EventsMessageProcessor eventsMessageProcessor,
            ReaderStatistics statistics,
            @Value("${storage.eventsPoolProcessor.queueCapacity}") int queueCapacity,
            @Value("${storage.eventsPoolProcessor.queueDrainLimit}") int queueDrainLimit,
            @Value("${storage.eventsPoolProcessor.numberOfThreads}") int numberOfThreads
    ) {
        return new EventsPoolProcessor(
                eventsMessageProcessor,
                statistics,
                queueCapacity,
                queueDrainLimit,
                numberOfThreads
        );
    }

    @Bean
    public EventsStreamReadProcessor eventsStreamReadProcessor(
            ReaderCache readerCache,
            ReaderStatistics statistics,
            EventsPoolProcessor eventsPoolProcessor
    ) {
        return new EventsStreamReadProcessor(
                readerCache, statistics, eventsPoolProcessor
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public LogbrokerStreamListener eventsStreamListener(
            EventsStreamReadProcessor eventsStreamReadProcessor,
            ReaderStatistics statistics,
            Clock clock,
            @Value("${storage.eventsStreamListener.readQueues}") int readQueues,
            @Value("${storage.eventsStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${storage.eventsStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "events",
                eventsStreamReadProcessor,
                statistics.getEvents(),
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public ReaderStreamConsumer eventsStreamConsumer(
            LogbrokerConfiguration eventsLogbrokerConfiguration,
            LogbrokerStreamListener eventsStreamListener,
            ReaderCache cache,
            @Value("${storage.eventsStreamConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${storage.eventsStreamConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ReaderStreamConsumer(
                eventsLogbrokerConfiguration,
                eventsStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }
}
