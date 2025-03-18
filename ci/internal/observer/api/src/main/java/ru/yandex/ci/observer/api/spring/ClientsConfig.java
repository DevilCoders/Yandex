package ru.yandex.ci.observer.api.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
public class ClientsConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public TvmTargetClientId arcanumTvmClientId(@Value("${observer.arcanumTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public HttpClientProperties arcanumHttpClientProperties(
            @Value("${observer.readOnlyArcanumClient.arcanumUrl}") String arcanumUrl,
            TvmClient tvmClient,
            TvmTargetClientId arcanumTvmClientId,
            CallsMonitorSource monitorSource
    ) {
        return HttpClientProperties.builder()
                .endpoint(arcanumUrl)
                .authProvider(new TvmAuthProvider(tvmClient, arcanumTvmClientId.getId()))
                .callsMonitorSource(monitorSource)
                .build();
    }

    @Bean
    public ArcanumClientImpl readOnlyArcanumClient(HttpClientProperties arcanumHttpClientProperties) {
        return ArcanumClientImpl.create(
                arcanumHttpClientProperties.withRetryPolicy(ArcanumClient.defaultRetryPolicy()),
                true
        );
    }
}
