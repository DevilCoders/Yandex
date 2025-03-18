package ru.yandex.ci.storage.post_processor.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.spring.AYamlerClientTestConfig;
import ru.yandex.ci.storage.core.spring.StorageCoreTestConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;

@Configuration
@Import({
        PostProcessorCacheConfig.class,
        StorageYdbTestConfig.class,
        StorageCoreTestConfig.class,
        AYamlerClientTestConfig.class,
        StoragePostProcessorPropertiesTestConfig.class
})
public class StoragePostProcessorTestConfig {
}
