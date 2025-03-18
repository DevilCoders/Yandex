package ru.yandex.ci.flow.spring;

import java.time.Clock;
import java.time.Duration;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;
import ru.yandex.ci.flow.engine.runtime.LaunchAutoReleaseDelegate;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.bazinga.JobWaitingScheduler;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarService;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.CasFlowStateUpdater;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchFactory;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchUpdateDelegate;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.StagedFlowStateUpdater;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.calculator.JobsMultiplyCalculator;
import ru.yandex.ci.flow.engine.runtime.state.calculator.ResourceProvider;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

/**
 * Конфигурация для запуска и управления задачами.
 * Не включает бины для непосредственного выполнения flow.
 */
// TODO: CI-3959
@Configuration
@Import({
        CommonServicesConfig.class,
        FlowZkConfig.class,
        FlowJobSchedulerConfig.class
})
public class FlowControlConfig {

    @Bean
    public FlowLaunchFactory flowLaunchFactory(
            SourceCodeService sourceCodeService,
            ResourceService resourceService
    ) {
        return new FlowLaunchFactory(resourceService, sourceCodeService);
    }

    @Bean
    public JobsMultiplyCalculator jobsTemplateCalculator(
            TaskletContextProcessor taskletContextProcessor,
            ResourceService resourceService
    ) {
        return new JobsMultiplyCalculator(taskletContextProcessor, resourceService);
    }

    @Bean
    public UpstreamResourcesCollector upstreamResourcesCollector(
            CiDb db,
            ResourceService resourceService
    ) {
        return new UpstreamResourcesCollector(db, resourceService);
    }

    @Bean
    public FlowStateCalculator flowStateCalculator(
            SourceCodeService sourceCodeService,
            ResourceProvider resourceProvider,
            UpstreamResourcesCollector upstreamResourcesCollector,
            JobsMultiplyCalculator jobsMultiplyCalculator,
            Clock clock,
            FlowLaunchMutexManager flowLaunchMutexManager
    ) {
        return new FlowStateCalculator(
                sourceCodeService,
                resourceProvider,
                upstreamResourcesCollector,
                jobsMultiplyCalculator,
                clock,
                flowLaunchMutexManager
        );
    }

    @Bean
    public CasFlowStateUpdater casFlowStateUpdater(
            FlowStateRevisionService stateRevisionService,
            FlowStateCalculator flowStateCalculator,
            JobScheduler jobScheduler,
            JobWaitingScheduler jobWaitingSchedulerImpl,
            JobInterruptionService jobInterruptionService,
            CiDb db,
            FlowLaunchUpdateDelegate updateDelegate,
            FlowLaunchMutexManager flowLaunchMutexManager) {
        return new CasFlowStateUpdater(
                stateRevisionService,
                flowStateCalculator,
                jobScheduler,
                jobWaitingSchedulerImpl,
                jobInterruptionService,
                db,
                updateDelegate,
                flowLaunchMutexManager
        );
    }

    @Bean
    public StagedFlowStateUpdater stagedFlowStateUpdater(
            FlowStateCalculator flowStateCalculator,
            JobScheduler jobScheduler,
            JobWaitingScheduler jobWaitingSchedulerImpl,
            FlowStateRevisionService stateRevisionService,
            JobInterruptionService jobInterruptionService,
            CiDb db,
            FlowLaunchUpdateDelegate updateDelegate,
            LaunchAutoReleaseDelegate launchAutoReleaseDelegate,
            FlowLaunchMutexManager flowLaunchMutexManager
    ) {
        return new StagedFlowStateUpdater(
                flowStateCalculator,
                jobScheduler,
                jobWaitingSchedulerImpl,
                stateRevisionService,
                jobInterruptionService,
                db,
                updateDelegate,
                launchAutoReleaseDelegate,
                flowLaunchMutexManager
        );
    }

    @Bean
    public FlowStateService flowStateService(
            FlowLaunchFactory flowLaunchFactory,
            StagedFlowStateUpdater stagedFlowStateUpdater,
            CasFlowStateUpdater casFlowStateUpdater,
            CiDb db
    ) {
        return new FlowStateService(
                flowLaunchFactory,
                stagedFlowStateUpdater,
                casFlowStateUpdater,
                db
        );
    }

    @Bean
    public JobProgressService jobProgressService(FlowStateService flowStateService, CiDb db) {
        return new JobProgressService(flowStateService, db);
    }

    @Bean
    public WorkCalendarService workCalendarService(WorkCalendarProvider workCalendarProvider) {
        return new WorkCalendarService(workCalendarProvider);
    }

    @Bean
    public FlowLaunchMutexManager flowLaunchMutexManager(
            CuratorFramework curatorFramework,
            @Value("${ci.flowLaunchMutexManager.mutexMapCapacity}") int mutexMapCapacity,
            @Value("${ci.flowLaunchMutexManager.concurrencyLevel}") int concurrencyLevel,
            @Value("${ci.flowLaunchMutexManager.acquireMutexTimeout}") Duration acquireMutexTimeout,
            @Value("${ci.flowLaunchMutexManager.oldMutexesCleanupTimeout}") Duration oldMutexesCleanupTimeout
    ) {
        return new FlowLaunchMutexManager(
                curatorFramework,
                mutexMapCapacity,
                concurrencyLevel,
                acquireMutexTimeout,
                oldMutexesCleanupTimeout
        );
    }

}
