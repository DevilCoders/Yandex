package ru.yandex.ci.core.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.sandbox.SandboxClient;

@Configuration
public class SandboxClientTestConfig {

    @Bean
    public SandboxClient sandboxClient() {
        return Mockito.mock(SandboxClient.class);
    }

}
