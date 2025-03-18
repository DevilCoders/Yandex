package ru.yandex.ci.core.spring;

import java.time.Duration;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;

@Configuration
@Import({CommonConfig.class, ArcClientConfig.class})
public class AYamlServiceConfig {

    @Bean
    public AYamlService aYamlService(
            ArcService arcService,
            @Nullable MeterRegistry meterRegistry,
            @Value("${ci.aYamlService.configCacheExpireAfterAccess}") Duration configCacheExpireAfterAccess,
            @Value("${ci.aYamlService.configCacheMaximumSize}") int configCacheMaximumSize
    ) {
        return new AYamlService(
                arcService,
                meterRegistry,
                configCacheExpireAfterAccess,
                configCacheMaximumSize
        );
    }
}
