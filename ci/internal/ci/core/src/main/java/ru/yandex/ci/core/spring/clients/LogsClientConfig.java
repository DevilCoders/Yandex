package ru.yandex.ci.core.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.logs.LogsClient;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
public class LogsClientConfig {

    @Bean
    public LogsClient logsClient(
            TvmClient tvmClient,
            TvmTargetClientId logsTvmClientId,
            @Value("${ci.logsClient.endpoint}") String endpoint,
            @Value("${ci.logsClient.connectTimeout}") Duration connectTimeout,
            @Value("${ci.logsClient.deadlineAfter}") Duration deadlineAfter
    ) {

        var properties = GrpcClientProperties.builder()
                .endpoint(endpoint)
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .plainText(false)
                .callCredentials(new TvmCallCredentials(tvmClient, logsTvmClientId.getId()))
                .build();

        return LogsClient.create(properties);
    }

    @Bean
    public TvmTargetClientId logsTvmClientId(@Value("${ci.logsTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }
}
