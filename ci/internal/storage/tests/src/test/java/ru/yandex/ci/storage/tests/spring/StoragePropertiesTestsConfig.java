package ru.yandex.ci.storage.tests.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.storage.api.spring.StorageApiPropertiesTestConfig;
import ru.yandex.ci.storage.core.spring.StorageCorePropertiesTestConfig;
import ru.yandex.ci.storage.post_processor.spring.StoragePostProcessorPropertiesTestConfig;
import ru.yandex.ci.storage.reader.spring.StorageReaderPropertiesTestConfig;
import ru.yandex.ci.storage.shard.spring.StorageShardPropertiesTestConfig;
import ru.yandex.ci.storage.tms.spring.StorageTmsPropertiesTestConfig;

@Configuration
@Import({
        CommonTestConfig.class,
        StorageCorePropertiesTestConfig.class,
        StorageApiPropertiesTestConfig.class,
        StorageReaderPropertiesTestConfig.class,
        StorageShardPropertiesTestConfig.class,
        StoragePostProcessorPropertiesTestConfig.class,
        StorageTmsPropertiesTestConfig.class
})
@PropertySource("classpath:ci-storage-tests.properties")
public class StoragePropertiesTestsConfig {
}
