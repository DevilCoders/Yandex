package ru.yandex.ci.tms.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.teamcity.TeamcityClient;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import({
        CommonConfig.class,
})
public class TeamcityClientConfig {

    @Bean
    public TeamcityClient teamcityClient(
            @Value("${ci.teamcityClient.url}") String url,
            @Value("${ci.teamcityClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .authProvider(new OAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return TeamcityClient.create(properties);
    }
}
