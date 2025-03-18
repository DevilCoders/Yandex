package ru.yandex.ci.observer.reader.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
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
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.ObserverStreamConsumer;
import ru.yandex.ci.observer.reader.message.events.ObserverEventsStreamQueuedReadProcessor;
import ru.yandex.ci.observer.reader.message.events.ObserverEventsStreamReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.registration.ObserverRegistrationProcessor;
import ru.yandex.ci.observer.reader.registration.RegistrationProcessorImpl;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.spring.stream.EventsStreamCoreConfig;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;

@Configuration
@Import({
        EventsStreamCoreConfig.class,
        ObserverInternalStreamConfig.class
})
public class ObserverReaderEventsConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public EventsStreamStatistics observerEventsStreamStatistics() {
        return new EventsStreamStatistics(meterRegistry, collectorRegistry);
    }

    @Bean
    public ObserverRegistrationProcessor observerRegistrationProcessor(CiObserverDb db, Clock clock) {
        return new RegistrationProcessorImpl(db, clock);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ObserverEventsStreamQueuedReadProcessor observerEventsStreamQueuedReadProcessor(
            ObserverCheckService checkService,
            ObserverInternalStreamWriter internalStreamWriter,
            EventsStreamStatistics observerEventsStreamStatistics,
            ObserverRegistrationProcessor observerRegistrationProcessor,
            @Value("${observer.observerEventsStreamQueuedReadProcessor.queueDrainLimit}") int queueDrainLimit,
            @Value("${observer.observerEventsStreamQueuedReadProcessor.queueMaxNumber}") int queueMaxNumber
    ) {
        return new ObserverEventsStreamQueuedReadProcessor(
                checkService, internalStreamWriter, observerEventsStreamStatistics,
                observerRegistrationProcessor, queueDrainLimit, queueMaxNumber, false
        );
    }

    @Bean
    public ObserverEventsStreamReadProcessor observerEventsStreamReadProcessor(
            EventsStreamStatistics statistics,
            ObserverEventsStreamQueuedReadProcessor queuedReadProcessor
    ) {
        return new ObserverEventsStreamReadProcessor(statistics, queuedReadProcessor);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public StreamListener eventsStreamListener(
            ObserverEventsStreamReadProcessor eventsStreamProcessor,
            EventsStreamStatistics statistics,
            Clock clock,
            @Value("${observer.eventsStreamListener.readQueues}") int readQueues,
            @Value("${observer.eventsStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${observer.eventsStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                StorageMetrics.PREFIX,
                "events",
                eventsStreamProcessor,
                statistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ObserverStreamConsumer eventsStreamConsumer(
            LogbrokerConfiguration eventsLogbrokerConfiguration,
            StreamListener eventsStreamListener,
            ObserverCache cache,
            @Value("${observer.eventsStreamConsumer.onlyNewData}") boolean onlyNewData,
            @Value("${observer.eventsStreamConsumer.maxUncommittedReads}") int maxUncommittedReads
    ) throws InterruptedException {
        return new ObserverStreamConsumer(
                eventsLogbrokerConfiguration,
                eventsStreamListener,
                cache.settings(),
                onlyNewData,
                maxUncommittedReads
        );
    }
}
