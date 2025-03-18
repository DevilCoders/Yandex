package ru.yandex.ci.core.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcTableClient;
import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
public class AbcClientConfig {

    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    @Bean
    public HttpClientProperties abcClientProperties(
            @Value("${ci.abcClient.apiUrl}") String apiUrl,
            TvmClient tvmClient,
            TvmTargetClientId abcTvmClientId,
            CallsMonitorSource monitorSource
    ) {
        return HttpClientProperties.builder()
                .endpoint(apiUrl)
                .authProvider(new TvmAuthProvider(tvmClient, abcTvmClientId.getId()))
                .callsMonitorSource(monitorSource)
                .build();
    }

    @Bean
    public TvmTargetClientId abcTvmClientId(@Value("${ci.abcTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public AbcClient abcClient(HttpClientProperties abcClientProperties) {
        return AbcClient.create(abcClientProperties);
    }

    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    @Bean
    public HttpClientProperties abcTableClientProperties(
            @Value("${ci.abcTableClient.ytUrl}") String ytUrl,
            @Value("${ci.abcTableClient.token}") String token
    ) {
        return HttpClientProperties.builder()
                .endpoint(ytUrl)
                .authProvider(new OAuthProvider(token))
                .followRedirects(true) // Redirect to heavy proxies
                .build();
    }

    @Bean
    public AbcTableClient abcTableClient(HttpClientProperties abcTableClientProperties) {
        return AbcTableClient.create(abcTableClientProperties);
    }
}
