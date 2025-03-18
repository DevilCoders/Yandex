package ru.yandex.ci.storage.reader.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-reader.properties")
@PropertySource("classpath:ci-storage-reader-junit-test.properties")
public class StorageReaderPropertiesTestConfig {
}
