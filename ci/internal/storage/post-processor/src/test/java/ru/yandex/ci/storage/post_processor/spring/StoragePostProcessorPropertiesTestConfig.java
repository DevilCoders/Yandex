package ru.yandex.ci.storage.post_processor.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-post-processor.properties")
@PropertySource("classpath:ci-storage-post-processor-junit-test.properties")
public class StoragePostProcessorPropertiesTestConfig {
}
