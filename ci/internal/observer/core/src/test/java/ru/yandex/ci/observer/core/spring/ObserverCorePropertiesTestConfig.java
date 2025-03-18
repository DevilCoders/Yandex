package ru.yandex.ci.observer.core.spring;


import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-observer-core/ci-observer.properties")
@PropertySource("classpath:ci-observer-core/ci-observer-junit-test.properties")
public class ObserverCorePropertiesTestConfig {
}
