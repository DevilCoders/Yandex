package ru.yandex.ci.flow.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.flow.utils.UrlService;

@Configuration
public class UrlServiceConfig {

    @Bean
    public UrlService urlService(@Value("${ci.urlService.arcanumUrl}") String arcanumUrl) {
        return new UrlService(arcanumUrl);
    }

}
