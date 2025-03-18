package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.mockserver.integration.ClientAndServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.tms.client.SolomonClient;

@Configuration
public class SolomonClientTestConfig {

    @Bean
    public ClientAndServer solomonServer() {
        return new ClientAndServer();
    }

    @Bean
    public SolomonClient solomonAlertClient(ClientAndServer solomonServer) {
        return SolomonClient.create(HttpClientPropertiesStub.of(solomonServer));
    }
}
