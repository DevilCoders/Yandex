package ru.yandex.ci.tms.spring.clients;

import org.mockserver.integration.ClientAndServer;
import org.springframework.beans.factory.config.ConfigurableBeanFactory;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Scope;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClientImpl;
import ru.yandex.ci.client.sandbox.SandboxClientProperties;

@Configuration
public class SandboxClientTestConfig {

    @Bean
    @Scope(ConfigurableBeanFactory.SCOPE_SINGLETON)
    public ClientAndServer sandboxServer() {
        return new ClientAndServer();
    }

    @Bean
    public SandboxClient sandboxClient(ClientAndServer sandboxServer) {
        var endpoint = HttpClientPropertiesStub.of(sandboxServer).getEndpoint();
        return SandboxClientImpl.create(
                SandboxClientProperties.builder()
                        .sandboxApiUrl(endpoint + "/v1.0/")
                        .sandboxApiV2Url(endpoint + "/v2/")
                        .httpClientProperties(HttpClientProperties.ofEndpoint(""))
                        .build()
        );
    }
}
