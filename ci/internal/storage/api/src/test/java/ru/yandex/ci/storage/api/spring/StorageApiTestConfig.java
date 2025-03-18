package ru.yandex.ci.storage.api.spring;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.api.controllers.StorageProxyApiController;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;

import static org.mockito.Mockito.mock;

@Configuration
@Import({StorageApiConfig.class})
public class StorageApiTestConfig {

    @Bean
    public StorageEventsProducer storageEventsProducer() {
        return mock(StorageEventsProducer.class);
    }

    @Bean
    public StorageProxyApiController storageProxyApiController() {
        return Mockito.mock(StorageProxyApiController.class);
    }

    @Bean
    public TestMetricsService metricsService() {
        return Mockito.mock(TestMetricsService.class);
    }

    @Bean
    public CiClient ciClient() {
        return mock(CiClient.class);
    }

    @Bean
    ArcService arcService() {
        return mock(ArcService.class);
    }

}
