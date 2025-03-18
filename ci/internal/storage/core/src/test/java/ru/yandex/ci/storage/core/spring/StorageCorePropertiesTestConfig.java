package ru.yandex.ci.storage.core.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-core/ci-storage.properties")
@PropertySource("classpath:ci-storage-core/ci-storage-unit-test.properties")
public class StorageCorePropertiesTestConfig {
}
