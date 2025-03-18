package ru.yandex.ci.event.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProperties;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.logbroker.LogbrokerTopics;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.event.arc.ArcReadProcessor;
import ru.yandex.ci.event.misc.EventReaderMetrics;
import ru.yandex.ci.logbroker.LogbrokerStatistics;
import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.logbroker.LogbrokerStreamConsumer;
import ru.yandex.ci.logbroker.LogbrokerStreamListener;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;


@Configuration
@Import({
        CommonConfig.class,
        LogbrokerConfig.class,
        ArcServiceConfig.class
})
public class ArcLogbrokerConfig {

    @Bean
    public LogbrokerStatistics arcLogbrokerStatistics(
            MeterRegistry meterRegistry,
            CollectorRegistry collectorRegistry) {
        return new LogbrokerStatisticsImpl(EventReaderMetrics.PREFIX, "arc", meterRegistry, collectorRegistry);
    }

    @Bean
    public StreamListener arcStreamListener(
            ArcReadProcessor readProcessor,
            LogbrokerStatistics arcLogbrokerStatistics,
            Clock clock,
            @Value("${ci.arcStreamListener.readQueues}") int readQueues,
            @Value("${ci.arcStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${ci.arcStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                EventReaderMetrics.PREFIX,
                "arc",
                readProcessor,
                arcLogbrokerStatistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    public LogbrokerConfiguration arcLogbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${ci.arcLogbrokerConfiguration.topic}") String topic,
            LogbrokerProperties logbrokerProperties,
            LogbrokerCredentialsProvider logbrokerCredentialsProvider
    ) {
        return new LogbrokerConfiguration(
                proxyBalancerHolder,
                LogbrokerTopics.parse(topic),
                logbrokerProperties,
                logbrokerCredentialsProvider
        );
    }

    @Bean
    public LogbrokerStreamConsumer arcLogbrokerStreamConsumer(
            LogbrokerConfiguration arcLogbrokerConfiguration,
            StreamListener arcStreamListener
    ) throws InterruptedException {
        return new LogbrokerStreamConsumer(arcLogbrokerConfiguration, arcStreamListener, false, 256);
    }
}
