package ru.yandex.ci.core.spring.clients;

import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.arc.ArcServiceImpl;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import(CommonConfig.class)
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class ArcClientConfig {
    @Bean
    public ArcServiceImpl arcService(
            MeterRegistry meterRegistry,
            @Value("${ci.arcService.endpoint}") String endpoint,
            @Value("${ci.arcService.connectTimeout}") Duration connectTimeout,
            @Value("${ci.arcService.deadlineAfter}") Duration deadlineAfter,
            @Value("${ci.arcService.oauthToken}") String oauthToken,
            @Value("${ci.arcService.processChangesAndSkipNotFoundCommits}") boolean processChangesAndSkipNotFoundCommits
    ) {
        var properties = GrpcClientProperties.builder()
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .endpoint(endpoint)
                .plainText(false)
                .callCredentials(new OAuthCallCredentials(oauthToken))
                .build();
        return new ArcServiceImpl(properties, meterRegistry, processChangesAndSkipNotFoundCommits);
    }
}
