package ru.yandex.ci.core.spring.clients;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.abc.AbcTableTestServer;
import ru.yandex.ci.client.abc.AbcTestServer;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;

@Configuration
public class AbcClientTestConfig {

    @Bean
    public AbcTestServer abcTestServer() {
        return new AbcTestServer();
    }

    @Bean
    public HttpClientProperties abcClientProperties(AbcTestServer abcTestServer) {
        return HttpClientPropertiesStub.of(abcTestServer.getServer());
    }

    @Bean
    public AbcTableTestServer abcTableTestServer() {
        return new AbcTableTestServer();
    }

    @Bean
    public HttpClientProperties abcTableClientProperties(AbcTableTestServer abcTableTestServer) {
        return HttpClientPropertiesStub.of(abcTableTestServer.getServer());
    }

}
