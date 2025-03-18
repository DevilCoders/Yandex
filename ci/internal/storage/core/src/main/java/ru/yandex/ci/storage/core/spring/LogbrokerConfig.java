package ru.yandex.ci.storage.core.spring;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProperties;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.kikimr.persqueue.auth.Credentials;
import ru.yandex.kikimr.persqueue.proxy.ProxyBalancer;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        TvmClientConfig.class
})
@Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
public class LogbrokerConfig {
    @Bean
    public LogbrokerProxyBalancerHolder logbrokerProxyBalancerHolder(
            LogbrokerProperties properties,
            @Value("${storage.logbrokerProxyBalancerHolder.keepAlive}") Duration keepAlive,
            @Value("${storage.logbrokerProxyBalancerHolder.keepAliveTimeout}") Duration keepAliveTimeout
    ) {
        return new LogbrokerProxyBalancerHolder(
                new ProxyBalancer(
                        properties.getHost(),
                        ProxyBalancer.DEFAULT_BALANCER_PORT,
                        ProxyBalancer.DEFAULT_PROXY_PORT,
                        (int) keepAlive.toSeconds(),
                        (int) keepAliveTimeout.toSeconds()
                )
        );
    }

    @Bean
    public TvmTargetClientId logbrokerTvmClientId(
            @Value("${storage.logbrokerTvmClientId.tvmId}") int tvmId
    ) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public LogbrokerProperties logbrokerProperties(
            @Value("${storage.logbrokerProperties.hostName}") String hostName,
            @Value("${storage.logbrokerProperties.consumer}") String consumer,
            TvmTargetClientId logbrokerTvmClientId
    ) {
        return new LogbrokerProperties(hostName, consumer, logbrokerTvmClientId.getId());
    }

    @Bean("storage.logbrokerCredentialsProvider")
    public LogbrokerCredentialsProvider logbrokerCredentialsProvider(
            LogbrokerProperties logbrokerProperties,
            TvmClient tvmClient
    ) {
        return () -> Credentials.tvm(tvmClient.getServiceTicketFor(logbrokerProperties.getLogbrokerTvmId()));
    }
}
