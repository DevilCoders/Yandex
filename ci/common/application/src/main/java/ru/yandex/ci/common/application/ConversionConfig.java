package ru.yandex.ci.common.application;

import org.springframework.boot.convert.ApplicationConversionService;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.convert.ConversionService;

@Configuration
public class ConversionConfig {

    @Bean
    public ConversionService conversionService() {
        return ApplicationConversionService.getSharedInstance();
    }
}
