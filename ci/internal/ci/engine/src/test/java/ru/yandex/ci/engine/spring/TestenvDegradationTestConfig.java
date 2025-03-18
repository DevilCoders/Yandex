package ru.yandex.ci.engine.spring;

import org.mockserver.integration.ClientAndServer;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.core.te.TestenvDegradationManager;

@Configuration
@Import(CommonTestConfig.class)
public class TestenvDegradationTestConfig {

    @Bean
    public ClientAndServer testenvServer() {
        return new ClientAndServer();
    }

    @Bean
    public TestenvDegradationManager testenvDegradationManager(ClientAndServer testenvServer) {
        return TestenvDegradationManager.create(HttpClientPropertiesStub.of(testenvServer));
    }
}
