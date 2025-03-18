package ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.storage;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.storage.api.spring.StorageApiCacheConfig;
import ru.yandex.ci.storage.api.spring.StorageApiTestConfig;
import ru.yandex.ci.storage.core.spring.ShardingConfig;
import ru.yandex.ci.storage.core.spring.StorageCoreTestConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;
import ru.yandex.ci.storage.tests.spring.StoragePropertiesTestsConfig;

@Configuration
@Import({
        CommonTestConfig.class,
        StorageCoreTestConfig.class,
        StorageYdbTestConfig.class,

        ShardingConfig.class,

        StorageApiCacheConfig.class,
        StorageApiTestConfig.class,
        StoragePropertiesTestsConfig.class,

        StorageApiGrpcTestConfig.class
})
public class StorageIntegrationTestConfig {

}
