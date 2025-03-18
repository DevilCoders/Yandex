package ru.yandex.ci.core.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClientImpl;
import ru.yandex.ci.client.sandbox.SandboxClientProperties;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;

@Import(CommonConfig.class)
@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class SandboxClientConfig {

    @Bean
    SandboxClientProperties sandboxClientProperties(
            @Value("${ci.sandboxClientProperties.apiUrl}") String apiUrl,
            @Value("${ci.sandboxClientProperties.apiV2Url}") String apiV2Url,
            @Value("${ci.sandboxClientProperties.oauthToken}") String oauthToken,
            @Value("${ci.sandboxClientProperties.maxLimitForGetTasksRequest}") Integer maxLimitForGetTasksRequest,
            CallsMonitorSource callsLogger
    ) {
        var clientProperties = HttpClientProperties.builder()
                .endpoint("")
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(callsLogger)
                .retryPolicy(SandboxClient.defaultRetryPolicy())
                .build();

        return SandboxClientProperties.builder()
                .sandboxApiUrl(apiUrl)
                .sandboxApiV2Url(apiV2Url)
                .httpClientProperties(clientProperties)
                .maxLimitForGetTasksRequest(
                        maxLimitForGetTasksRequest != null
                                ? maxLimitForGetTasksRequest
                                : SandboxClient.MAX_LIMIT_FOR_GET_TASKS_REQUEST
                )
                .build();
    }

    @Bean
    public SandboxClient sandboxClient(SandboxClientProperties properties) {
        return SandboxClientImpl.create(properties);
    }
}
