package ru.yandex.ci.engine.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetryPolicies;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClientImpl;
import ru.yandex.ci.client.sandbox.SandboxClientProperties;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.client.yav.YavClientImpl;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class SecurityClientsConfig {

    @Bean
    public SandboxClient securitySandboxClient(
            @Value("${ci.securitySandboxClient.url}") String url,
            @Value("${ci.securitySandboxClient.urlV2}") String urlV2,
            TvmClient tvmClient,
            TvmTargetClientId sandboxClientId,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint("")
                .authProvider(new TvmAuthProvider(tvmClient, sandboxClientId.getId()))
                .retryPolicy(SandboxClient.defaultRetryPolicy())
                .callsMonitorSource(monitorSource)
                .build();
        return SandboxClientImpl.create(
                SandboxClientProperties.builder()
                        .sandboxApiUrl(url)
                        .sandboxApiV2Url(urlV2)
                        .httpClientProperties(properties)
                        .build()
        );
    }

    @Bean
    public YavClient yavClient(
            @Value("${ci.yavClient.host}") String host,
            @Value("${ci.yavClient.tvmSelfClientId}") int tvmSelfClientId,
            TvmTargetClientId yavClientId,
            TvmClient tvmClient,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(host)
                .authProvider(new TvmAuthProvider(tvmClient, yavClientId.getId()))
                .retryPolicy(RetryPolicies.requireAll(
                        RetryPolicies.idempotentMethodsOnly(),
                        RetryPolicies.retryWithSleep(10, Duration.ofSeconds(1))
                ))
                .callsMonitorSource(monitorSource)
                .build();
        return YavClientImpl.create(properties, tvmSelfClientId);
    }

    @Bean
    public TvmTargetClientId sandboxClientId(@Value("${ci.sandboxClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public TvmTargetClientId yavClientId(@Value("${ci.yavClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }
}
