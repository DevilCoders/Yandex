package ru.yandex.ci.storage.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.db.CiStorageDb;

@Configuration
@Import({CommonConfig.class, StorageYdbConfig.class})
//@Profile(CiProfile.NOT_UNIT_TEST_PROFILE) //TODO CI-3227 uncomment
@Profile(CiProfile.TESTING_OR_LOCAL_PROFILE) //TODO CI-3227 remove
public class StorageTemporalServiceConfig {

    @Bean
    public TemporalService temporalService(
            MeterRegistry meterRegistry,
            CiStorageDb ciStorageDb,
            @Value("${storage.temporalService.endpoint}") String endpoint,
            @Value("${storage.temporalService.namespace}") String namespace,
            @Value("${storage.temporalService.uiBaseUrl}") String uiBaseUrl
    ) {
        return TemporalConfigurationUtil.createTemporalService(
                endpoint, namespace, ciStorageDb, meterRegistry, uiBaseUrl
        );
    }

}
