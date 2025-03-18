package ru.yandex.ci.engine.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
public class StorageClientConfig {

    @Bean
    public StorageApiClient storageApiClient(GrpcClientProperties storageGrpcClientProperties) {
        return StorageApiClient.create(storageGrpcClientProperties);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public TvmTargetClientId storageTvmClientId(@Value("${ci.storageTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public GrpcClientProperties storageGrpcClientProperties(
            @Value("${ci.storageApiClient.endpoint}") String url,
            @Value("${ci.storageApiClient.connectTimeout}") Duration connectTimeout,
            @Value("${ci.storageApiClient.deadlineAfter}") Duration deadlineAfter,
            TvmClient tvmClient,
            TvmTargetClientId storageTvmClientId
    ) {
        return GrpcClientProperties.builder()
                .endpoint(url)
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .callCredentials(new TvmCallCredentials(tvmClient, storageTvmClientId.getId()))
                .build();
    }

}
