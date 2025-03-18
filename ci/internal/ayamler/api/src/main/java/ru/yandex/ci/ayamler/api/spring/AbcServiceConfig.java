package ru.yandex.ci.ayamler.api.spring;

import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.ayamler.AbcService;
import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.core.spring.clients.AbcClientConfig;

@Configuration
@Import(AbcClientConfig.class)
public class AbcServiceConfig {

    @Bean
    public AbcService abcService(
            AbcClient abcClient,
            MeterRegistry meterRegistry,
            @Value("${ayamler.abcService.serviceMembersCacheExpireAfterWrite}")
                    Duration serviceMembersCacheExpireAfterWrite,
            @Value("${ayamler.abcService.cacheConcurrencyLevel}") int cacheConcurrencyLevel
    ) {
        return new AbcService(
                abcClient,
                serviceMembersCacheExpireAfterWrite,
                meterRegistry,
                cacheConcurrencyLevel
        );
    }

}
