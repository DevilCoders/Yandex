package ru.yandex.ci.tms.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.rm.RmClient;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import({
        CommonConfig.class,
})
public class RmClientConfig {

    @Bean
    public RmClient rmClient(
            @Value("${ci.rmClient.url}") String url,
            CallsMonitorSource monitorSource
    ) {
        return RmClient.create(HttpClientProperties.ofEndpoint(url, monitorSource));
    }
}
