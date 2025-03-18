package ru.yandex.ci.engine.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.base.http.retries.RetryWithExponentialDelayPolicy;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class TestenvClientConfig {

    @Bean
    public TestenvClient testenvClient(
            @Value("${ci.testenvClient.url}") String url,
            @Value("${ci.testenvClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .requestTimeout(Duration.ofMinutes(3))
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return TestenvClient.create(properties);
    }

    @Bean
    public TestenvClient testenvClientWithLongRetry(
            @Value("${ci.testenvClientWithLongRetry.url}") String url,
            @Value("${ci.testenvClientWithLongRetry.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .requestTimeout(Duration.ofMinutes(3))
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .retryPolicy(new RetryWithExponentialDelayPolicy(20, Duration.ofSeconds(1), Duration.ofSeconds(30)))
                .clientNameSuffix("-LongRetry")
                .build();
        return TestenvClient.create(properties);
    }
}
