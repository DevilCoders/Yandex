package ru.yandex.ci.observer.tests.spring;

import java.time.Clock;
import java.time.Duration;
import java.util.List;
import java.util.stream.IntStream;

import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Primary;

import ru.yandex.ci.observer.api.spring.ObserverApiConfig;
import ru.yandex.ci.observer.api.statistics.aggregated.AggregatedStatisticsService;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.message.events.ObserverEventsStreamQueuedReadProcessor;
import ru.yandex.ci.observer.reader.message.events.ObserverEventsStreamReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.InternalStreamStatistics;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamMessageProcessor;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamQueuedReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamReadProcessor;
import ru.yandex.ci.observer.reader.message.internal.writer.ObserverInternalStreamWriter;
import ru.yandex.ci.observer.reader.registration.ObserverRegistrationProcessor;
import ru.yandex.ci.observer.reader.spring.ObserverReaderConfig;
import ru.yandex.ci.observer.tests.logbroker.ObserverTopic;
import ru.yandex.ci.observer.tests.logbroker.TestCommonLogbrokerService;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerWriterFactoryImpl;


@Configuration
@Import({
        ObserverReaderConfig.class,
        ObserverApiConfig.class,
})
public class TestObserverConfig {
    @Bean
    public ObserverInternalStreamQueuedReadProcessor observerInternalStreamQueuedReadProcessor(
            InternalStreamStatistics statistics,
            ObserverInternalStreamMessageProcessor messageProcessor
    ) {
        return new ObserverInternalStreamQueuedReadProcessor(
                statistics, messageProcessor, 256, 1, true
        );
    }

    // Intentionally left without import, see ContextHierarchy in ObserverTestsTestBase
    @Bean
    public LogbrokerWriterFactory observerInternalLogbrokerWriterFactory(
            TestCommonLogbrokerService logbrokerService
    ) {
        return new TestLogbrokerWriterFactoryImpl(ObserverTopic.INTERNAL, "internal", logbrokerService);
    }

    @Bean
    public String observerLogbrokerServiceReadersSetter(
            TestCommonLogbrokerService logbrokerService,
            MainStreamReadProcessor observerMainStreamReadProcessor,
            ObserverEventsStreamReadProcessor observerEventsStreamReadProcessor,
            ObserverInternalStreamReadProcessor observerInternalStreamReadProcessor
    ) {
        logbrokerService.registerMainConsumer(observerMainStreamReadProcessor);
        logbrokerService.registerEventsConsumer(observerEventsStreamReadProcessor);
        logbrokerService.registerObserverInternalConsumer(observerInternalStreamReadProcessor);

        return "";
    }

    @Bean
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
                observerRegistrationProcessor, queueDrainLimit, queueMaxNumber, true
        );
    }

    @Bean
    public AggregatedStatisticsService aggregatedStatisticsService(
            Clock clock,
            CiObserverDb ciObserverDb,
            @Value("${observer.aggregatedStatisticsService.percentileLevels}") int[] percentileLevels,
            @Value("${observer.aggregatedStatisticsService.stagesToAggregate}") String[] stagesToAggregate,
            @Value("${observer.aggregatedStatisticsService.aggregateWindows}") Duration[] aggregateWindows,
            @Value("${observer.aggregatedStatisticsService.lastSaveDelay}") Duration lastSaveDelay
    ) {
        return new AggregatedStatisticsService(
                clock,
                ciObserverDb,
                IntStream.of(percentileLevels).boxed().toList(),
                List.of(stagesToAggregate),
                List.of(aggregateWindows),
                lastSaveDelay
        );
    }

    @Bean
    @Primary
    public CollectorRegistry collectorRegistry() {
        // продовый бин использует default registry, чтобы подцепить метрики которые пишут из либ
        // этот registry - статическое поле, что приводит к конфликту в storage-observer тестах
        return new CollectorRegistry(true);
    }

}
