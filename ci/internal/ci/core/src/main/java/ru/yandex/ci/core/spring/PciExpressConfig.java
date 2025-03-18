package ru.yandex.ci.core.spring;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.pciexpress.PciExpressClient;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class PciExpressConfig {

    @Bean
    public GrpcClientProperties pciExpressClientProperties(
            @Value("${ci.pciExpressClientProperties.endpoint}") String endpoint,
            @Value("${ci.pciExpressClientProperties.connectTimeout}") Duration connectTimeout,
            TvmClient tvmClient,
            TvmTargetClientId pciExpressTvmClientId
    ) {
        return GrpcClientProperties.builder()
                .endpoint(endpoint)
                .connectTimeout(connectTimeout)
                .callCredentials(new TvmCallCredentials(tvmClient, pciExpressTvmClientId.getId()))
                .build();
    }

    @Bean
    public TvmTargetClientId pciExpressTvmClientId(@Value("${ci.pciExpressTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public PciExpressClient pciExpressClient(GrpcClientProperties pciExpressClientProperties) {
        return PciExpressClient.create(pciExpressClientProperties);
    }

}
