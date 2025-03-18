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
import ru.yandex.ci.event.arcanum.ArcanumReadProcessor;
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
        ArcanumServiceConfig.class
})
public class ArcanumLogbrokerConfig {

    @Bean
    public LogbrokerStatistics arcanumLogbrokerStatistics(
            MeterRegistry meterRegistry,
            CollectorRegistry collectorRegistry) {
        return new LogbrokerStatisticsImpl(EventReaderMetrics.PREFIX, "arcanum", meterRegistry, collectorRegistry);
    }

    @Bean
    public StreamListener arcanumStreamListener(
            ArcanumReadProcessor readProcessor,
            LogbrokerStatistics arcanumLogbrokerStatistics,
            Clock clock,
            @Value("${ci.arcanumStreamListener.readQueues}") int readQueues,
            @Value("${ci.arcanumStreamListener.readQueueCapacity}") int readQueueCapacity,
            @Value("${ci.arcanumStreamListener.readDrainLimit}") int readDrainLimit
    ) {
        return new LogbrokerStreamListener(
                EventReaderMetrics.PREFIX,
                "arcanum",
                readProcessor,
                arcanumLogbrokerStatistics,
                clock,
                readQueues,
                readQueueCapacity,
                readDrainLimit
        );
    }

    @Bean
    public LogbrokerConfiguration arcanumLogbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${ci.arcanumLogbrokerConfiguration.topic}") String topic,
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
    public LogbrokerStreamConsumer arcanumLogbrokerStreamConsumer(
            LogbrokerConfiguration arcanumLogbrokerConfiguration,
            StreamListener arcanumStreamListener
    ) throws InterruptedException {
        return new LogbrokerStreamConsumer(arcanumLogbrokerConfiguration, arcanumStreamListener, false, 256);
    }
}
