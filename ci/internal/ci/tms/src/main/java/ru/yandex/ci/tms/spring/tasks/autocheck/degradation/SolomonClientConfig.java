package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.tms.client.SolomonClient;

@Configuration
@Import(CommonConfig.class)
public class SolomonClientConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public SolomonClient solomonAlertClient(
            @Value("${ci.solomonAlertClient.host}") String host,
            @Value("${ci.solomonAlertClient.oauthToken}") String oauthToken,
            CallsMonitorSource callsMonitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(host)
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(callsMonitorSource)
                .build();
        return SolomonClient.create(properties);
    }
}
