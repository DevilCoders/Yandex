package ru.yandex.ci.tms.spring.bazinga;

import java.util.concurrent.TimeUnit;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.common.bazinga.monitoring.BazingaScheduledMetrics;
import ru.yandex.ci.common.bazinga.monitoring.CronTasksFailurePercent;
import ru.yandex.ci.common.bazinga.monitoring.OnetimeJobFailurePercent;

@Configuration
@Import(BazingaMonitoringConfig.class)
@EnableScheduling
public class BazingaMonitoringSchedule {

    @Autowired
    BazingaScheduledMetrics bazingaScheduledMetrics;
    @Autowired
    CronTasksFailurePercent cronTasksFailurePercent;
    @Autowired
    OnetimeJobFailurePercent onetimeJobFailurePercent;

    @Scheduled(
            fixedRateString = "${ci.BazingaMonitoringSchedule.updateBazingaScheduledMetricsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void updateBazingaScheduledMetrics() {
        bazingaScheduledMetrics.run();
        cronTasksFailurePercent.run();
        onetimeJobFailurePercent.run();
    }
}
