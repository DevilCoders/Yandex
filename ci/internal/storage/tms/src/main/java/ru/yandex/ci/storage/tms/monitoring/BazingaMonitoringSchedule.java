package ru.yandex.ci.storage.tms.monitoring;

import java.util.concurrent.TimeUnit;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.monitoring.BazingaScheduledMetrics;
import ru.yandex.ci.common.bazinga.monitoring.CronTasksFailurePercent;
import ru.yandex.ci.common.bazinga.monitoring.OnetimeJobFailurePercent;

@Configuration
@Import(MonitoringConfig.class)
@EnableScheduling
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class BazingaMonitoringSchedule {

    @Autowired
    private BazingaScheduledMetrics bazingaScheduledMetrics;
    @Autowired
    private CronTasksFailurePercent cronTasksFailurePercent;
    @Autowired
    private OnetimeJobFailurePercent onetimeJobFailurePercent;

    @Scheduled(
            fixedRateString = "${storage.BazingaMonitoringSchedule.updateBazingaScheduledMetricsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void updateBazingaScheduledMetrics() {
        bazingaScheduledMetrics.run();
        cronTasksFailurePercent.run();
        onetimeJobFailurePercent.run();
    }
}
