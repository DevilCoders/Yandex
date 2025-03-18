package ru.yandex.ci.storage.core.spring;


import java.time.Clock;

import io.prometheus.client.CollectorRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryEmptyImpl;
import ru.yandex.ci.storage.core.utils.TimeTraceService;

@Configuration
@Import({
        CommonTestConfig.class,
        StorageCorePropertiesTestConfig.class
})
public class StorageCoreTestConfig {

    @Bean
    public TimeTraceService timeTraceService(Clock clock, CollectorRegistry collectorRegistry) {
        return new TimeTraceService(clock, collectorRegistry);
    }

    @Bean("storageEventLogbrokerWriterFactory")
    public LogbrokerWriterFactory prestableOrTestingStorageEventLogbrokerWriterFactory() {
        return new LogbrokerWriterFactoryEmptyImpl("events");
    }
}
