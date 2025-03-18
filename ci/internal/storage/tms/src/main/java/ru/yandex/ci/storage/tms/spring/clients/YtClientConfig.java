package ru.yandex.ci.storage.tms.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.ci.storage.core.yt.impl.YtClientFactoryImpl;
import ru.yandex.yt.ytclient.rpc.RpcCredentials;

@Configuration
public class YtClientConfig {

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public YtClientFactory ytClientFactory(
            @Value("${storage.ytClientFactory.username}") String username,
            @Value("${storage.ytClientFactory.token}") String token
    ) {
        return new YtClientFactoryImpl(new RpcCredentials(username, token));
    }
}
