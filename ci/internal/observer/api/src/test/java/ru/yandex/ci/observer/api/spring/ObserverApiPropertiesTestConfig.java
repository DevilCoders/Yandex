package ru.yandex.ci.observer.api.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-observer-api.properties")
public class ObserverApiPropertiesTestConfig {
}
