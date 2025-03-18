package ru.yandex.ci.storage.core.spring.stream;

import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProperties;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.core.logbroker.LogbrokerTopics;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;

@Configuration
@Import({
        LogbrokerConfig.class,
})
@Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
public class MainStreamCoreConfig {
    @Bean
    public LogbrokerConfiguration mainStreamLogbrokerConfiguration(
            LogbrokerProxyBalancerHolder proxyBalancerHolder,
            @Value("${storage.mainStreamLogbrokerConfiguration.topics}") String topics,
            LogbrokerProperties logbrokerProperties,
            @Qualifier("storage.logbrokerCredentialsProvider") LogbrokerCredentialsProvider credentialsProvider
    ) {
        return new LogbrokerConfiguration(
                proxyBalancerHolder,
                LogbrokerTopics.parse(topics),
                logbrokerProperties,
                credentialsProvider
        );
    }
}
