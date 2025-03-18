package ru.yandex.ci.api.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.controllers.autocheck.AutocheckController;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.spring.AutocheckConfig;

@Configuration
@Import(AutocheckConfig.class)
public class AutocheckApiConfig {

    @Bean
    public AutocheckController autocheckController(AutocheckService autocheckService) {
        return new AutocheckController(autocheckService);
    }
}
