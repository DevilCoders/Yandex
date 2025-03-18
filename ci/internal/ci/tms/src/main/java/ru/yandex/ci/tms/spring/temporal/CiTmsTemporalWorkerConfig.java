package ru.yandex.ci.tms.spring.temporal;

import java.time.Duration;
import java.util.List;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ClassPathScanningCandidateComponentProvider;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.core.type.filter.AssignableTypeFilter;

import ru.yandex.ci.common.temporal.TemporalCronUtils;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.ActivityCronImplementation;
import ru.yandex.ci.common.temporal.config.ActivityImplementation;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.config.TemporalSystemConfig;
import ru.yandex.ci.common.temporal.config.TemporalWorkerFactoryWrapper;
import ru.yandex.ci.common.temporal.config.WorkflowCronImplementation;
import ru.yandex.ci.common.temporal.config.WorkflowImplementation;
import ru.yandex.ci.common.temporal.heartbeat.TemporalWorkerHeartbeatService;
import ru.yandex.ci.common.temporal.monitoring.TemporalMonitoringService;
import ru.yandex.ci.common.temporal.worflow.TemporalCiSystemCronWorkflow;
import ru.yandex.ci.flow.spring.tasks.FlowTasksTemporalConfig;
import ru.yandex.ci.flow.spring.temporal.CiTemporalServiceConfig;

@Configuration
@Import({
        CiTemporalServiceConfig.class,
        TemporalSystemConfig.class,
        FlowTasksTemporalConfig.class,
})
public class CiTmsTemporalWorkerConfig {

    @Bean(initMethod = "start", destroyMethod = "shutdown")
    public TemporalWorkerFactoryWrapper workerFactory(
            TemporalService temporalService,
            @Value("${ci.workerFactory.defaultMaxWorkflowThreads}") int defaultMaxWorkflowThreads,
            @Value("${ci.workerFactory.defaultMaxActivityThreads}") int defaultMaxActivityThreads,
            @Value("${ci.workerFactory.cronMaxWorkflowThreads}") int cronMaxWorkflowThreads,
            @Value("${ci.workerFactory.cronMaxActivityThreads}") int cronMaxActivityThreads,
            List<ActivityImplementation> activitiesImplementations,
            List<ActivityCronImplementation> activitiesCronImplementations,
            TemporalWorkerHeartbeatService heartbeatService,
            TemporalMonitoringService monitoringService
    ) {
        temporalService.registerCronTask(
                TemporalCiSystemCronWorkflow.class,
                wf -> wf::run,
                TemporalCronUtils.everyMinute(),
                Duration.ofMinutes(5)
        );

        var workflowImplementations = classesWithInterface(WorkflowImplementation.class);
        var workflowCronImplementations = classesWithInterface(WorkflowCronImplementation.class);

        return TemporalConfigurationUtil.createWorkerFactoryBuilder(temporalService)
                .heartbeatService(heartbeatService)
                .monitoringService(monitoringService)
                .worker(TemporalConfigurationUtil.createDefaultWorker()
                        .maxWorkflowThreads(defaultMaxWorkflowThreads)
                        .maxActivityThreads(defaultMaxActivityThreads)
                        .workflowImplementationTypes(workflowImplementations)
                        .activitiesImplementations(activitiesImplementations.toArray()))
                .worker(TemporalConfigurationUtil.createCronWorker()
                        .maxWorkflowThreads(cronMaxWorkflowThreads)
                        .maxActivityThreads(cronMaxActivityThreads)
                        .workflowImplementationTypes(workflowCronImplementations)
                        .activitiesImplementations(activitiesCronImplementations.toArray()))
                .build();
    }

    private static Class<?>[] classesWithInterface(Class<?> interfaceClass) {
        var scanner = new ClassPathScanningCandidateComponentProvider(false);
        scanner.addIncludeFilter(new AssignableTypeFilter(interfaceClass));

        return scanner.findCandidateComponents("ru.yandex.ci")
                .stream()
                .map(bd -> {
                    try {
                        return Class.forName(bd.getBeanClassName());
                    } catch (ClassNotFoundException e) {
                        throw new RuntimeException(e);
                    }
                })
                .toArray(Class<?>[]::new);
    }
}
