package ru.yandex.ci.storage.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryEmptyImpl;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducerImpl;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;

@Configuration
@Import({
        CommonConfig.class,
        LogbrokerConfig.class
})
public class StorageEventProducerConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public StorageEventsProducer storageEventProducer(
            @Value("${storage.stableStorageEventProducer.numberOfPartitions}") int numberOfPartitions,
            LogbrokerWriterFactory storageEventLogbrokerWriterFactory
    ) {
        return new StorageEventsProducerImpl(numberOfPartitions, meterRegistry, storageEventLogbrokerWriterFactory);
    }

    @Bean("storageEventLogbrokerWriterFactory")
    @Profile(CiProfile.STABLE_PROFILE)
    public LogbrokerWriterFactory stableStorageEventLogbrokerWriterFactory(
            LogbrokerProxyBalancerHolder proxyHolder,
            @Qualifier("storage.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider,
            @Value("${storage.stableStorageEventProducer.topic}") String topic
    ) {
        return new LogbrokerWriterFactoryImpl(
                topic,
                "events",
                new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                credentialsProvider
        );
    }

    @Bean("storageEventLogbrokerWriterFactory")
    @Profile(CiProfile.PRESTABLE_OR_TESTING_PROFILE)
    public LogbrokerWriterFactory prestableOrTestingStorageEventLogbrokerWriterFactory() {
        return new LogbrokerWriterFactoryEmptyImpl("events");
    }
}
