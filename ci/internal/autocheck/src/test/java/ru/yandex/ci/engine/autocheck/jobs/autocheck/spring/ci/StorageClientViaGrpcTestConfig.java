package ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.ci;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.engine.spring.clients.StorageClientConfig;

@Configuration
@Import(StorageClientConfig.class)
public class StorageClientViaGrpcTestConfig {

    @Bean
    public GrpcClientProperties storageGrpcClientProperties(String storageServerName) {
        return GrpcClientPropertiesStub.of(storageServerName);
    }

}
