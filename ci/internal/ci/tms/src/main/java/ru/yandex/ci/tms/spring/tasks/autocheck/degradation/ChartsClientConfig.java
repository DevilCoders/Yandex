package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.charts.ChartsClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;

@Import(CommonConfig.class)
@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class ChartsClientConfig {

    @Bean
    public ChartsClient chartsClient(
            @Value("${ci.chartsClient.url}") String url,
            @Value("${ci.chartsClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return ChartsClient.create(properties);
    }
}
