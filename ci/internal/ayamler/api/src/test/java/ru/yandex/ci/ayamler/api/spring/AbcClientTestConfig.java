package ru.yandex.ci.ayamler.api.spring;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcTableClient;

@Configuration
public class AbcClientTestConfig {

    @Bean
    public AbcClient abcClient() {
        return Mockito.mock(AbcClient.class);
    }

    @Bean
    public AbcTableClient abcTableClient() {
        return Mockito.mock(AbcTableClient.class);
    }
}
