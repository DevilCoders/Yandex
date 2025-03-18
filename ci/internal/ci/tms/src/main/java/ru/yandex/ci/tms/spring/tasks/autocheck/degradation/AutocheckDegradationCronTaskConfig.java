package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import java.time.Duration;
import java.util.Set;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.engine.spring.TestenvDegradationConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationCronTask;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationMonitoringsSource;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationNotificationsManager;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationPostcommitManager;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationTaskInactivityConditionsChecker;

@Configuration
@Import({
        CommonConfig.class,
        AutocheckDegradationMonitoringsSourceConfig.class,
        AutocheckDegradationPostcommitManagerConfig.class,
        AutocheckDegradationNotificationsManagerConfig.class,
        FlowZkConfig.class,
        TestenvDegradationConfig.class
})
public class AutocheckDegradationCronTaskConfig {

    @Value("${ci.AutocheckDegradationCronTaskConfig.dryRun}")
    boolean dryRun;

    @Bean
    public AutocheckDegradationCronTask autocheckDegradationTask(
            @Value("${ci.autocheckDegradationTask.taskInterval}") Duration taskInterval,
            @Value("${ci.autocheckDegradationTask.postcommitsDegradation}") boolean postcommitsDegradation,
            @Value("${ci.autocheckDegradationTask.precommitsDegradationPlatforms}")
                    TestenvDegradationManager.Platform[] precommitsDegradationPlatforms,
            AutocheckDegradationPostcommitManager postcommitManager,
            AutocheckDegradationMonitoringsSource monitoringsSource,
            TestenvDegradationManager teDegradationManager,
            AutocheckDegradationNotificationsManager notificationsManager,
            AutocheckDegradationTaskInactivityConditionsChecker inactivityConditionsChecker,
            CuratorFramework curator) {

        return new AutocheckDegradationCronTask(
                taskInterval,
                dryRun,
                Set.of(precommitsDegradationPlatforms),
                postcommitsDegradation,
                postcommitManager,
                monitoringsSource,
                teDegradationManager,
                notificationsManager,
                inactivityConditionsChecker,
                curator
        );
    }
}
