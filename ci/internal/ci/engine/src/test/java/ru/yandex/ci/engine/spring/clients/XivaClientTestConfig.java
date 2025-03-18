package ru.yandex.ci.engine.spring.clients;

import org.mockserver.integration.ClientAndServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.xiva.XivaClient;
import ru.yandex.ci.client.xiva.XivaClientImpl;

@Configuration
public class XivaClientTestConfig {

    @Bean
    public ClientAndServer xivaServer() {
        return new ClientAndServer();
    }

    @Bean
    public XivaClient xivaClient(ClientAndServer xivaServer) {
        return XivaClientImpl.create("ci-unit-test", HttpClientPropertiesStub.of(xivaServer));
    }

}
