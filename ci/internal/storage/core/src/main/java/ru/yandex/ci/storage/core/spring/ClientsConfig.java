package ru.yandex.ci.storage.core.spring;

import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.ci.CiClientImpl;
import ru.yandex.ci.client.oldci.OldCiClient;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.arc.ArcServiceImpl;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class ClientsConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public OldCiClient oldCiClient(
            @Value("${storage.oldCiClient.url}") String url,
            @Value("${storage.oldCiClient.token}") String token,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .authProvider(new OAuthProvider(token))
                .callsMonitorSource(monitorSource)
                .build();
        return OldCiClient.create(properties);
    }

    @Bean
    public ArcServiceImpl arcService(
            @Value("${storage.arcService.endpoint}") String endpoint,
            @Value("${storage.arcService.connectTimeout}") Duration connectTimeout,
            @Value("${storage.arcService.token}") String token,
            @Value("${storage.arcService.processChangesAndSkipNotFoundCommits}")
                    boolean processChangesAndSkipNotFoundCommits
    ) {
        var properties = GrpcClientProperties.builder()
                .endpoint(endpoint)
                .connectTimeout(connectTimeout)
                .plainText(false)
                .callCredentials(new OAuthCallCredentials(token))
                .build();
        return new ArcServiceImpl(properties, meterRegistry, processChangesAndSkipNotFoundCommits);
    }

    @Bean
    public TestenvClient testenvClient(
            @Value("${storage.testenvClient.url}") String url,
            @Value("${storage.testenvClient.oAuthToken}") String oAuthToken,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .authProvider(new OAuthProvider(oAuthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return TestenvClient.create(properties);
    }

    @Bean
    public TvmTargetClientId ciTvmClientId(@Value("${storage.ciTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public CiClient ciClient(
            TvmClient tvmClient,
            TvmTargetClientId ciTvmClientId,
            @Value("${storage.ciClient.endpoint}") String endpoint,
            @Value("${storage.ciClient.userAgent}") String userAgent,
            @Value("${storage.ciClient.connectTimeout}") Duration connectTimeout,
            @Value("${storage.ciClient.deadlineAfter}") Duration deadlineAfter
    ) {
        var properties = GrpcClientProperties.builder()
                .endpoint(endpoint)
                .userAgent(userAgent)
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .callCredentials(new TvmCallCredentials(tvmClient, ciTvmClientId.getId()))
                .build();
        return CiClientImpl.create(properties);
    }
}
