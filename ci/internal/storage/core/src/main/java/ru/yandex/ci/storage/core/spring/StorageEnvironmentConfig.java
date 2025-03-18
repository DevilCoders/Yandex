package ru.yandex.ci.storage.core.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.storage.core.model.StorageEnvironment;

@Configuration
public class StorageEnvironmentConfig {
    @Bean
    public StorageEnvironment storageEnvironment(@Value("${storage.environment}") String environment) {
        return new StorageEnvironment(environment);
    }
}
