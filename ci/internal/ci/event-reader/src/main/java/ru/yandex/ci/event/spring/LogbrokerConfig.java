package ru.yandex.ci.event.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProperties;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.kikimr.persqueue.auth.Credentials;
import ru.yandex.kikimr.persqueue.proxy.ProxyBalancer;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import(TvmClientConfig.class)
public class LogbrokerConfig {

    @Bean
    public TvmTargetClientId logbrokerTvmClientId(
            @Value("${ci.logbrokerTvmClientId.tvmId}") int tvmId
    ) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public LogbrokerCredentialsProvider logbrokerCredentialsProvider(
            TvmClient tvmClient,
            TvmTargetClientId logbrokerTvmClientId
    ) {
        return () -> Credentials.tvm(tvmClient.getServiceTicketFor(logbrokerTvmClientId.getId()));
    }

    @Bean
    public LogbrokerProperties logbrokerProperties(
            @Value("${ci.logbrokerProperties.hostName}") String hostName,
            @Value("${ci.logbrokerProperties.consumer}") String consumer,
            TvmTargetClientId logbrokerTvmClientId
    ) {
        return new LogbrokerProperties(hostName, consumer, logbrokerTvmClientId.getId());
    }

    @Bean
    public LogbrokerProxyBalancerHolder logbrokerProxyBalancerHolder(
            LogbrokerProperties logbrokerProperties
    ) {
        return new LogbrokerProxyBalancerHolder(new ProxyBalancer(logbrokerProperties.getHost()));
    }

}
