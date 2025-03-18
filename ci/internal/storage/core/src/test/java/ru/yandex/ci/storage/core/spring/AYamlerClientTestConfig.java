package ru.yandex.ci.storage.core.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.ayamler.AYamlerClient;

import static org.mockito.Mockito.mock;

@Configuration
@Import({
        AYamlerClientConfig.class
})
public class AYamlerClientTestConfig {

    @Bean
    AYamlerClient aYamlerClient() {
        return mock(AYamlerClient.class);
    }

}
