package ru.yandex.ci.engine.spring.clients;

import java.time.Duration;
import java.util.Set;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetryPolicies;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.xiva.XivaClient;
import ru.yandex.ci.client.xiva.XivaClientImpl;
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
public class XivaClientConfig {

    @Bean
    public XivaClient xivaClient(
            @Value("${ci.xivaClient.service}") String service,
            @Value("${ci.xivaClient.endpoint}") String endpoint,
            TvmClient tvmClient,
            TvmTargetClientId xivaTvmClientId,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(endpoint)
                .authProvider(new TvmAuthProvider(tvmClient, xivaTvmClientId.getId()))
                .callsMonitorSource(monitorSource)
                .retryPolicy(RetryPolicies.requireAll(
                        RetryPolicies.nonRetryableResponseCodes(Set.of(400, 405)),
                        RetryPolicies.retryWithSleep(6, Duration.ofMillis(200))
                ))
                .build();
        return XivaClientImpl.create(service, properties);
    }

    @Bean
    public TvmTargetClientId xivaTvmClientId(@Value("${ci.xivaTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }
}
