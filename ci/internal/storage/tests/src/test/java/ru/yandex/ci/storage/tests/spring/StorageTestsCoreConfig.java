package ru.yandex.ci.storage.tests.spring;

import org.mockito.Mockito;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.storage.api.controllers.StorageProxyApiController;
import ru.yandex.ci.storage.core.spring.bazinga.SingleThreadBazingaServiceConfig;

@Configuration
@Import({
        StorageTestsCoreBaseConfig.class,
        SingleThreadBazingaServiceConfig.class
})
public class StorageTestsCoreConfig {

    @Bean
    public StorageProxyApiController storageProxyApiController() {
        return Mockito.mock(StorageProxyApiController.class);
    }

}
