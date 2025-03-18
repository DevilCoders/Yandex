package ru.yandex.ci.storage.tms.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.core.cache.DefaultStorageCache;
import ru.yandex.ci.storage.core.large.AutocheckTasksFactory;

@Configuration
@Import({
        StorageTmsCacheConfig.class,
        StorageTmsPropertiesTestConfig.class
})
public class LargeTasksTestConfig {

    @Bean
    public AutocheckTasksFactory largeTasksSearch(DefaultStorageCache storageApiCache) {
        return new AutocheckTasksFactory(storageApiCache);
    }
}
