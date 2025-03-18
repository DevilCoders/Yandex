package ru.yandex.ci.common.temporal.config;

import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.MetricTemporalMonitoringService;
import ru.yandex.ci.common.temporal.monitoring.TemporalMonitoringService;
import ru.yandex.ci.common.temporal.worflow.TemporalLostWorkflowActivityImpl;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;

@Configuration
public class TemporalSystemConfig {

    @Bean
    public TemporalLostWorkflowActivityImpl temporalLostWorkflowActivity(TemporalService temporalService,
                                                                         TemporalDb temporalDb,
                                                                         MeterRegistry meterRegistry) {
        return new TemporalLostWorkflowActivityImpl(temporalService, temporalDb, meterRegistry);
    }

    @Bean(initMethod = "startAsync")
    public TemporalWorkerHeartbeatService temporalWorkerHeartbeatService(
            @Value("${temporal.temporalWorkerHeartbeatService.heartBeatInterval}") Duration heartBeatInterval,
            @Value("${temporal.temporalWorkerHeartbeatService.heartBeatThreadCount}") int heartBeatThreadCount,
            MeterRegistry meterRegistry
    ) {
        return new TemporalWorkerHeartbeatService(
                heartBeatInterval,
                heartBeatThreadCount,
                meterRegistry
        );
    }

    @Bean(initMethod = "startAsync")
    public TemporalMonitoringService temporalMonitoringService(
            MeterRegistry meterRegistry,
            TemporalDb temporalDb,
            @Value("${temporal.temporalMonitoringService.activityAttemptCountForWarn}") int activityAttemptCountForWarn,
            @Value("${temporal.temporalMonitoringService.activityAttemptCountForCrit}") int activityAttemptCountForCrit
    ) {
        return new MetricTemporalMonitoringService(
                meterRegistry,
                temporalDb,
                activityAttemptCountForWarn,
                activityAttemptCountForCrit
        );
    }

}
