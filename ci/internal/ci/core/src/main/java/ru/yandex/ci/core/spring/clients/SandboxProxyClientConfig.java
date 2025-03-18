package ru.yandex.ci.core.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.client.sandbox.ProxySandboxClientImpl;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;

@Import(CommonConfig.class)
@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class SandboxProxyClientConfig {

    @Bean
    public ProxySandboxClient proxySandboxClient(
            @Value("${ci.proxySandboxClient.endpoint}") String endpoint,
            @Value("${ci.proxySandboxClient.oauthToken}") String oauthToken,
            CallsMonitorSource callsLogger
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(endpoint)
                .authProvider(new OAuthProvider(oauthToken))
                .retryPolicy(SandboxClient.defaultRetryPolicy())
                .callsMonitorSource(callsLogger)
                .build();
        return ProxySandboxClientImpl.create(properties);
    }
}
