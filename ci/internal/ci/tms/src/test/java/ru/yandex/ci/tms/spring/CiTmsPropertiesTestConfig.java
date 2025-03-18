package ru.yandex.ci.tms.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-tms.properties")
@PropertySource("classpath:ci-tms-unit-test.properties")
public class CiTmsPropertiesTestConfig {
}
