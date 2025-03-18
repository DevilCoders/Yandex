package ru.yandex.ci.storage.shard.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.spring.AYamlerClientTestConfig;
import ru.yandex.ci.storage.core.spring.StorageCoreTestConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;

@Configuration
@Import({
        StorageShardCacheConfig.class,
        StorageShardStatisticsConfig.class,

        StorageYdbTestConfig.class,
        StorageCoreTestConfig.class,
        AYamlerClientTestConfig.class,
        StorageShardPropertiesTestConfig.class
})
public class StorageShardTestConfig {
}
