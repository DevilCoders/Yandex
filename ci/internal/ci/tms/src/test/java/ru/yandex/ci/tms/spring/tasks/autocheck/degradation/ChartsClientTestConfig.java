package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.mockserver.integration.ClientAndServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.charts.ChartsClient;

@Configuration
public class ChartsClientTestConfig {

    @Bean
    public ClientAndServer chartsServer() {
        return new ClientAndServer();
    }

    @Bean
    public ChartsClient chartsClient(ClientAndServer chartsServer) {
        return ChartsClient.create(HttpClientPropertiesStub.of(chartsServer));
    }
}
