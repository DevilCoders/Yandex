package ru.yandex.ci.storage.exporter.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.config.TemporalSystemConfig;
import ru.yandex.ci.common.temporal.config.TemporalWorkerFactoryWrapper;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.TemporalMonitoringService;
import ru.yandex.ci.storage.core.spring.StorageTemporalServiceConfig;
import ru.yandex.ci.storage.core.yt.YtExportWorkflow;
import ru.yandex.ci.storage.core.yt.impl.YtExportActivityImpl;
import ru.yandex.ci.storage.core.yt.impl.YtExportWorkflowImpl;

@Configuration
@Import({
        StorageTemporalServiceConfig.class,
        TemporalSystemConfig.class,
        StorageTemporalYtExporterActivityConfig.class,
})
public class StorageExporterTemporalWorkerConfig {


    @Bean(initMethod = "start", destroyMethod = "shutdown")
    public TemporalWorkerFactoryWrapper workerFactory(
            TemporalService temporalService,
            TemporalWorkerHeartbeatService temporalWorkerHeartbeatService,
            TemporalMonitoringService temporalMonitoringService,
            @Value("${storage.workerFactory.maxYtExportThreads}") int maxYtExportThreads,
            YtExportActivityImpl ytExportActivityImpl
    ) {

        return TemporalConfigurationUtil.createWorkerFactoryBuilder(temporalService)
                .heartbeatService(temporalWorkerHeartbeatService)
                .monitoringService(temporalMonitoringService)
                .worker(TemporalConfigurationUtil.createWorker(YtExportWorkflow.QUEUE)
                        .maxWorkflowThreads(maxYtExportThreads)
                        .maxActivityThreads(maxYtExportThreads)
                        .workflowImplementationTypes(YtExportWorkflowImpl.class)
                        .activitiesImplementations(ytExportActivityImpl))
                .build();
    }
}
