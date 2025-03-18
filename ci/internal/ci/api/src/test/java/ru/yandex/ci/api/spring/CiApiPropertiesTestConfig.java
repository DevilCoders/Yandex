package ru.yandex.ci.api.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-api.properties")
@PropertySource("classpath:ci-api-unit-test.properties")
public class CiApiPropertiesTestConfig {
}
