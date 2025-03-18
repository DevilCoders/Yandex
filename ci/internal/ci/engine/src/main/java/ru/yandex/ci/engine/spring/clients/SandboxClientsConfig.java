package ru.yandex.ci.engine.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.engine.flow.SandboxClientFactory;
import ru.yandex.ci.engine.flow.SandboxClientFactoryImpl;

@Configuration
@Import(CommonConfig.class)
public class SandboxClientsConfig {

    @Bean
    public SandboxClientFactory sandboxClientFactory(
            @Value("${ci.sandboxClientFactory.url}") String url,
            @Value("${ci.sandboxClientFactory.urlV2}") String urlV2,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint("")
                .retryPolicy(SandboxClient.defaultRetryPolicy())
                .callsMonitorSource(monitorSource)
                .build();
        return new SandboxClientFactoryImpl(url, urlV2, properties);
    }
}
