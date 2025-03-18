package ru.yandex.ci.storage.api;

import io.grpc.BindableService;
import io.grpc.ManagedChannel;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.api.spring.StorageApiCacheConfig;
import ru.yandex.ci.storage.api.spring.StorageApiPropertiesTestConfig;
import ru.yandex.ci.storage.core.StorageYdbTestBase;

@ContextConfiguration(classes = {
        StorageApiCacheConfig.class,
        StorageApiPropertiesTestConfig.class
})
public abstract class ApiTestBase extends StorageYdbTestBase {

    @Autowired
    protected StorageApiCache apiCache;

    protected ManagedChannel buildChannel(BindableService service) {
        return GrpcTestUtils.buildChannel(service);
    }
}
