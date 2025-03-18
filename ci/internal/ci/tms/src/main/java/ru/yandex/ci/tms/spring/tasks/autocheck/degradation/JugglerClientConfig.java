package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.juggler.JugglerClient;
import ru.yandex.ci.client.juggler.JugglerPushClient;
import ru.yandex.ci.client.juggler.JugglerPushClientImpl;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;

@Import(CommonConfig.class)
@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class JugglerClientConfig {
    @Bean
    public JugglerClient jugglerClient(
            @Value("${ci.jugglerClient.host}") String host,
            @Value("${ci.jugglerClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(host)
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return JugglerClient.create(properties);
    }

    @Bean
    public JugglerPushClient jugglerPushClient(
            @Value("${ci.jugglerPushClient.host}") String host,
            @Value("${ci.jugglerPushClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(host)
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return JugglerPushClientImpl.create(properties);
    }
}
