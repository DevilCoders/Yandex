package ru.yandex.ci.engine.spring.clients;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.testenv.TestenvClient;

@Configuration
public class TestenvClientTestConfig {

    @Bean
    public TestenvClient testenvClient() {
        return Mockito.mock(TestenvClient.class);
    }

    @Bean
    public TestenvClient testenvClientWithLongRetry() {
        return Mockito.mock(TestenvClient.class);
    }
}
