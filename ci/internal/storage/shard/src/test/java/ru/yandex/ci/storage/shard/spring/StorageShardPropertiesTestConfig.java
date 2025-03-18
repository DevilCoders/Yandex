package ru.yandex.ci.storage.shard.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-shard.properties")
@PropertySource("classpath:ci-storage-shard-unit-test.properties")
public class StorageShardPropertiesTestConfig {
}
