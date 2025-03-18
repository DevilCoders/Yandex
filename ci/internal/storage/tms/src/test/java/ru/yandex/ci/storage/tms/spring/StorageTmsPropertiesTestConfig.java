package ru.yandex.ci.storage.tms.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

@Configuration
@PropertySource("classpath:ci-storage-tms.properties")
public class StorageTmsPropertiesTestConfig {
}
