package ru.yandex.ci.engine.spring.clients;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.arcanum.ArcanumTestServer;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;

@Configuration
@Import(ArcanumClientConfig.class)
public class ArcanumClientTestConfig {

    @Bean
    ArcanumTestServer arcanumTestServer() {
        return new ArcanumTestServer();
    }

    @Bean
    public HttpClientProperties arcanumHttpClientProperties(ArcanumTestServer arcanumTestServer) {
        return HttpClientPropertiesStub.of(arcanumTestServer.getServer());
    }

}
