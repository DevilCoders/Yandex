package ru.yandex.ci.engine.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.yav.YavClient;

@Configuration
public class SecurityClientsTestConfig {

    @Bean
    public SandboxClient securitySandboxClient() {
        return Mockito.mock(SandboxClient.class);
    }

    @Bean
    public YavClient yavClient() {
        return Mockito.mock(YavClient.class);
    }
}
