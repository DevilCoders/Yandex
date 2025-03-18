package ru.yandex.ci.engine.spring;

import java.net.UnknownHostException;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.core.logbroker.LogbrokerWriterImpl;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.kikimr.persqueue.auth.Credentials;
import ru.yandex.kikimr.persqueue.proxy.ProxyBalancer;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import(TvmClientConfig.class)
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class LogbrokerCiEventConfig {

    @Bean
    public LogbrokerProxyBalancerHolder logbrokerProxyBalancerHolder(
            @Value("${ci.logbrokerProxyBalancerHolder.host}") String host
    ) {
        return new LogbrokerProxyBalancerHolder(new ProxyBalancer(host));
    }

    @Bean
    public LogbrokerWriter logbrokerLaunchEventWriter(
            LogbrokerProxyBalancerHolder logbrokerProxyBalancerHolder,
            @Value("${ci.logbrokerLaunchEventWriter.topic}") String topic,
            @Qualifier("engine.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) throws UnknownHostException {
        return new LogbrokerWriterImpl(topic, logbrokerProxyBalancerHolder, credentialsProvider);
    }

    @Bean
    public LogbrokerWriter logbrokerCiEventWriter(
            LogbrokerProxyBalancerHolder logbrokerProxyBalancerHolder,
            @Value("${ci.logbrokerCiEventWriter.topic}") String topic,
            @Qualifier("engine.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) throws UnknownHostException {
        return new LogbrokerWriterImpl(topic, logbrokerProxyBalancerHolder, credentialsProvider);
    }

    @Bean
    public TvmTargetClientId logbrokerTvmClientId(
            @Value("${ci.logbrokerTvmClientId.tvmId}") int tvmId
    ) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean("engine.logbrokerCredentialsProvider")
    public LogbrokerCredentialsProvider eventTvmCredentialsProvider(
            TvmClient tvmClient,
            TvmTargetClientId logbrokerTvmClientId
    ) {
        return () -> Credentials.tvm(tvmClient.getServiceTicketFor(logbrokerTvmClientId.getId()));
    }

}
