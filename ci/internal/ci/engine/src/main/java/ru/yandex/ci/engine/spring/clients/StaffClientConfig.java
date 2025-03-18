package ru.yandex.ci.engine.spring.clients;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class,
})
public class StaffClientConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public TvmTargetClientId staffTvmClientId(
            @Value("${ci.staffTvmClientId.tvmId}") int tvmId
    ) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public StaffClient staffClient(
            @Value("${ci.staffClient.staffUrl}") String staffUrl,
            TvmTargetClientId staffTvmClientId,
            TvmClient tvmClient,
            CallsMonitorSource monitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(staffUrl)
                .authProvider(new TvmAuthProvider(tvmClient, staffTvmClientId.getId()))
                .callsMonitorSource(monitorSource)
                .build();
        return StaffClient.create(properties);
    }

}
