package ru.yandex.ci.tms.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.engine.flow.SandboxClientFactory;
import ru.yandex.ci.tms.test.SandboxClientTaskletExecutorStub;

@Configuration
public class SandboxClientsStubConfig {
    @Bean
    public SandboxClient sandboxClient() {
        return Mockito.spy(new SandboxClientTaskletExecutorStub());
    }

    @Bean
    public SandboxClientFactory sandboxClientFactory(SandboxClient sandboxClient) {
        return (SandboxClientTaskletExecutorStub) sandboxClient;
    }
}
