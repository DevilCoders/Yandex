package ru.yandex.ci.flow.spring.temporal;

import java.time.ZoneId;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.logs.LogsClient;
import ru.yandex.ci.common.application.CiApplication;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.logging.TemporalLogService;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.LogsClientConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import({
        CommonConfig.class,
        YdbCiConfig.class,
        LogsClientConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class CiTemporalServiceConfig {

    @Bean
    public TemporalService temporalService(
            MeterRegistry meterRegistry,
            CiDb ciDb,
            @Value("${ci.temporalService.endpoint}") String endpoint,
            @Value("${ci.temporalService.namespace}") String namespace,
            @Value("${ci.temporalService.uiBaseUrl}") String uiBaseUrl
    ) {
        return TemporalConfigurationUtil.createTemporalService(endpoint, namespace, ciDb, meterRegistry, uiBaseUrl);
    }

    @Bean
    public TemporalLogService temporalLogService(TemporalService temporalService, LogsClient logsClient) {
        return new TemporalLogService(
                temporalService,
                logsClient,
                "ci",
                "ci-tms",
                CiApplication.getApplicationEnvironment(),
                ZoneId.systemDefault()
        );
    }

}
