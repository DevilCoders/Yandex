package ru.yandex.ci.tms.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.tsum.TsumAuthProvider;
import ru.yandex.ci.client.tsum.TsumClient;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import({
        CommonConfig.class,
})
public class TsumClientConfig {

    @Bean
    public TsumClient tsumClient(
            @Value("${ci.tsumClient.url}") String url,
            @Value("${ci.tsumClient.oauthToken}") String oauthToken,
            CallsMonitorSource monitorSource) {
        var properties = HttpClientProperties.builder()
                .endpoint(url)
                .authProvider(new TsumAuthProvider(oauthToken))
                .callsMonitorSource(monitorSource)
                .build();
        return TsumClient.create(properties);
    }
}
