package ru.yandex.ci.core.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;

@Configuration
public class SandboxProxyClientTestConfig {
    @Bean
    public ProxySandboxClient proxySandboxClient() {
        return Mockito.mock(ProxySandboxClient.class);
    }
}
