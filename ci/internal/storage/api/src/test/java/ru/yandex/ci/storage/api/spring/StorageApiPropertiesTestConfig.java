package ru.yandex.ci.storage.api.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-api.properties")
@PropertySource("classpath:ci-storage-api-unit-test.properties")
public class StorageApiPropertiesTestConfig {
}
