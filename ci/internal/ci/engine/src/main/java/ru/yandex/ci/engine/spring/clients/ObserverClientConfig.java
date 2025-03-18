package ru.yandex.ci.engine.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.observer.ObserverClient;
import ru.yandex.ci.client.observer.ObserverClientImpl;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import({
        CommonConfig.class
})
public class ObserverClientConfig {

    @Bean
    public ObserverClient observerClient(
            @Value("${ci.observerClient.apiUrl}") String apiUrl,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(apiUrl)
                .callsMonitorSource(monitorSource)
                .build();
        return ObserverClientImpl.create(properties);
    }

}
