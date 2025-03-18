package ru.yandex.ci.observer.reader.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-observer-reader.properties")
@PropertySource("classpath:ci-observer-reader-unit-test.properties")
public class ObserverReaderPropertiesTestConfig {
}
