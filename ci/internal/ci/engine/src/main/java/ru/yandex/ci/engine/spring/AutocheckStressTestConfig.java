package ru.yandex.ci.engine.spring;

import java.time.Clock;
import java.time.Duration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.jvm.ExecutorServiceMetrics;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.scheduling.concurrent.ThreadPoolTaskExecutor;

import ru.yandex.ci.client.observer.ObserverClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StressTestCancelAllFlowsJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StressTestRevisionsFindJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StressTestRevisionsLaunchJob;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.FlowLaunchService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.spring.clients.ObserverClientConfig;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;

@Configuration
@Import({
        AutocheckConfig.class,
        ObserverClientConfig.class,
        LaunchConfig.class,
        FlowExecutionConfig.class,
})
public class AutocheckStressTestConfig {

    @Bean
    public StressTestRevisionsFindJob stressTestFindTargetRevisionsJob(
            ObserverClient observerClient
    ) {
        return new StressTestRevisionsFindJob(observerClient);
    }

    @Bean
    public ThreadPoolTaskExecutor stressTestLaunchExecutor(
            MeterRegistry meterRegistry,
            @Value("${ci.stressTestLaunchExecutor.corePoolSize}") int corePoolSize,
            @Value("${ci.stressTestLaunchExecutor.maxPoolSize}") int maxPoolSize,
            @Value("${ci.stressTestLaunchExecutor.awaitTermination}") Duration awaitTermination
    ) {
        var executor = new ThreadPoolTaskExecutor() {
            @Override
            protected ExecutorService initializeExecutor(ThreadFactory threadFactory,
                                                         RejectedExecutionHandler rejectedExecutionHandler) {
                var executorService = super.initializeExecutor(threadFactory, rejectedExecutionHandler);
                return ExecutorServiceMetrics.monitor(meterRegistry, executorService,
                        "stress_test_start_launch_executor");
            }
        };
        executor.setCorePoolSize(corePoolSize);
        executor.setMaxPoolSize(maxPoolSize);
        executor.setThreadNamePrefix("stress_test_start_launch-");
        executor.setWaitForTasksToCompleteOnShutdown(true);
        executor.setAwaitTerminationMillis(awaitTermination.toMillis());
        return executor;
    }

    @Bean
    public StressTestRevisionsLaunchJob stressTestRevisionsLaunchJob(
            ObserverClient observerClient,
            LaunchService launchService,
            ConfigurationService configurationService,
            CiMainDb db,
            Clock clock,
            ThreadPoolTaskExecutor stressTestLaunchExecutor
    ) {
        return new StressTestRevisionsLaunchJob(observerClient, launchService, configurationService, db, clock,
                stressTestLaunchExecutor);
    }

    @Bean
    public StressTestCancelAllFlowsJob stressTestCancelAllFlowsJob(
            AutocheckService autocheckService,
            FlowLaunchService flowLaunchService,
            FlowLaunchMutexManager flowLaunchMutexManager,
            CiMainDb db
    ) {
        return new StressTestCancelAllFlowsJob(autocheckService, flowLaunchService, flowLaunchMutexManager, db);
    }

}
