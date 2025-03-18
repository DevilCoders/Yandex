package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.mockserver.integration.ClientAndServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.JugglerPushClient;
import ru.yandex.ci.client.juggler.JugglerPushClientImpl;

@Configuration
public class JugglerClientTestConfig {

    @Bean
    public ClientAndServer jugglerServer() {
        return new ClientAndServer();
    }

    @Bean
    public JugglerClient jugglerClient(ClientAndServer jugglerServer) {
        return JugglerClient.create(HttpClientPropertiesStub.of(jugglerServer));
    }

    @Bean
    public JugglerPushClient jugglerPushClient(ClientAndServer jugglerServer) {
        return JugglerPushClientImpl.create(HttpClientPropertiesStub.of(jugglerServer));
    }
}
