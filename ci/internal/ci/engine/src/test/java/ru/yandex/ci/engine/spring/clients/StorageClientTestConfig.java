package ru.yandex.ci.engine.spring.clients;

import java.util.UUID;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.storage.StorageApiTestServer;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;

@Configuration
@Import(StorageClientConfig.class)
public class StorageClientTestConfig {

    @Bean
    public String storageGrpcClientChannel() {
        return "storageGrpcClient-" + UUID.randomUUID();
    }

    @Bean
    public StorageApiTestServer storageApiTestServer(String storageGrpcClientChannel) {
        return new StorageApiTestServer(storageGrpcClientChannel);
    }

    @Bean
    public GrpcClientProperties storageGrpcClientProperties(String storageGrpcClientChannel) {
        return GrpcClientPropertiesStub.of(storageGrpcClientChannel);
    }

}
