package ru.yandex.ci.tms.spring.bazinga;

import java.time.Duration;

import com.google.common.base.Preconditions;
import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;
import org.springframework.scheduling.annotation.EnableScheduling;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.monitoring.BazingaScheduledMetrics;
import ru.yandex.ci.common.bazinga.monitoring.CronTasksFailurePercent;
import ru.yandex.ci.common.bazinga.monitoring.HeartbeatCronTask;
import ru.yandex.ci.common.bazinga.monitoring.HeartbeatOneTimeTask;
import ru.yandex.ci.common.bazinga.monitoring.OnetimeJobFailurePercent;
import ru.yandex.ci.common.bazinga.monitoring.OnetimeJobRetries;
import ru.yandex.commune.bazinga.BazingaControllerAndWorkerApps;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.BazingaWorkerApp;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;

@Configuration
@Import(BazingaServiceConfig.class)
@EnableScheduling
public class BazingaMonitoringConfig {

    @Autowired
    MeterRegistry meterRegistry;

    @Bean
    public HeartbeatOneTimeTask heartbeatOneTimeTask() {
        return new HeartbeatOneTimeTask(meterRegistry);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public HeartbeatCronTask heartbeatCronTask(
            BazingaTaskManager bazingaTaskManager,
            @Value("${ci.heartbeatCronTask.taskInterval}") Duration taskInterval
    ) {
        return new HeartbeatCronTask(bazingaTaskManager, taskInterval);
    }

    @Bean
    public BazingaScheduledMetrics bazingaScheduledMetrics(
            BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps
    ) {
        return new BazingaScheduledMetrics(bazingaControllerAndWorkerApps, meterRegistry);
    }

    @Bean
    public CronTasksFailurePercent cronTasksFailurePercent(
            BazingaControllerApp controllerApp,
            BazingaStorage bazingaStorage,
            @Value("${ci.cronTasksFailurePercent.failureCountToWarn}") int failureCountToWarn
    ) {
        return new CronTasksFailurePercent(controllerApp, bazingaStorage, failureCountToWarn, meterRegistry);
    }

    @Bean
    public OnetimeJobFailurePercent onetimeJobFailurePercentage(
            BazingaControllerApp controllerApp,
            BazingaStorage bazingaStorage
    ) {
        return new OnetimeJobFailurePercent(
                controllerApp,
                ensureYdbBazingaStorage(bazingaStorage),
                meterRegistry);
    }

    @Bean
    public OnetimeJobRetries onetimeJobRetries(
            BazingaControllerApp controllerApp,
            BazingaStorage bazingaStorage,
            BazingaWorkerApp bazingaWorkerApp
    ) {
        return new OnetimeJobRetries(
                controllerApp,
                ensureYdbBazingaStorage(bazingaStorage),
                bazingaWorkerApp.getWorkerTaskRegistry(),
                meterRegistry,
                42
        );
    }

    private static YdbBazingaStorage ensureYdbBazingaStorage(BazingaStorage bazingaStorage) {
        Preconditions.checkState(bazingaStorage instanceof YdbBazingaStorage,
                "Support only YdbBazingaStorage class, found %s", bazingaStorage.getClass());
        return (YdbBazingaStorage) bazingaStorage;
    }

}
