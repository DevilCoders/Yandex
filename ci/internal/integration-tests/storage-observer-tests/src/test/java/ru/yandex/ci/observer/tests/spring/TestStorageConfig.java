package ru.yandex.ci.observer.tests.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.api.spring.StorageApiConfig;
import ru.yandex.ci.storage.post_processor.spring.StoragePostProcessorConfig;
import ru.yandex.ci.storage.reader.spring.StorageReaderConfig;
import ru.yandex.ci.storage.shard.spring.StorageShardConfig;
import ru.yandex.ci.storage.tests.spring.StorageTestsCoreConfig;
import ru.yandex.ci.storage.tms.spring.StorageTmsConfig;

@Configuration
@Import({
        StorageTestsCoreConfig.class,
        StorageApiConfig.class,
        StorageReaderConfig.class,
        StorageShardConfig.class,
        StorageTmsConfig.class,
        StoragePostProcessorConfig.class
})
public class TestStorageConfig {
}
