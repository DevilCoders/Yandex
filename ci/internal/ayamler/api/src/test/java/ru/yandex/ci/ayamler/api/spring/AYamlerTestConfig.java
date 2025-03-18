package ru.yandex.ci.ayamler.api.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-ayamler-api.properties")
@PropertySource("classpath:ci-ayamler-api-unit-test.properties")
public class AYamlerTestConfig {
}
