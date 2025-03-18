package ru.yandex.ci.ayamler.api.spring;

import io.prometheus.client.CollectorRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.ayamler.AYamlerService;
import ru.yandex.ci.ayamler.api.controllers.AYamlerController;

@Configuration
@Import({
        AYamlerServiceConfig.class,
})
public class AYamlerApiConfig {

    @Bean
    public AYamlerController aYamlerController(
            AYamlerService aYamlerService,
            CollectorRegistry collectorRegistry
    ) {
        return new AYamlerController(aYamlerService, collectorRegistry);
    }

}
