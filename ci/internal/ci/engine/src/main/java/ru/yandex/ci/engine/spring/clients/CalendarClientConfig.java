package ru.yandex.ci.engine.spring.clients;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.retries.RetryWithExponentialDelayPolicy;
import ru.yandex.ci.client.calendar.CalendarClient;
import ru.yandex.ci.client.calendar.CalendarClientImpl;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
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
public class CalendarClientConfig {

    @Bean
    public CalendarClient calendarClient(
            TvmClient tvmClient,
            CallsMonitorSource monitorSource,
            TvmTargetClientId calendarClientId,
            @Value("${ci.calendarClient.calendarUrl}") String calendarUrl
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(calendarUrl)
                .authProvider(new TvmAuthProvider(tvmClient, calendarClientId.getId()))
                .retryPolicy(new RetryWithExponentialDelayPolicy(20, Duration.ofSeconds(1), Duration.ofSeconds(30)))
                .callsMonitorSource(monitorSource)
                .build();
        return CalendarClientImpl.create(properties);
    }

    @Bean
    public TvmTargetClientId calendarClientId(@Value("${ci.calendarClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }
}
