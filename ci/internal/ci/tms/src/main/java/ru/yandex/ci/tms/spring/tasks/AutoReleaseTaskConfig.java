package ru.yandex.ci.tms.spring.tasks;

import java.time.Clock;
import java.time.Duration;
import java.util.concurrent.TimeUnit;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseCronTask;
import ru.yandex.ci.engine.launch.auto.AutoReleaseMetrics;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.PostponeActionCronTask;
import ru.yandex.ci.engine.launch.auto.PostponeActionService;
import ru.yandex.ci.engine.spring.DiscoveryConfig;
import ru.yandex.ci.tms.autorelease.GraphDiscoveryResultReadinessCronTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@EnableScheduling
@Import({
        DiscoveryConfig.class,
})
public class AutoReleaseTaskConfig {

    @Autowired
    private AutoReleaseMetrics autoReleaseMetrics;

    @Scheduled(
            fixedRateString = "${ci.AutoReleaseTaskConfig.refreshAutoReleaseMetricsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void refreshAutoReleaseMetrics() {
        autoReleaseMetrics.run();
    }

    @Bean
    public GraphDiscoveryResultReadinessCronTask graphDiscoveryResultReadinessCronTask(
            CiMainDb db,
            GraphDiscoveryService graphDiscoveryService,
            BazingaTaskManager bazingaTaskManager,
            @Value("${ci.graphDiscoveryResultReadinessCronTask.runDelay}") Duration runDelay,
            @Value("${ci.graphDiscoveryResultReadinessCronTask.timeout}") Duration timeout,
            Clock clock,
            CuratorFramework curator
    ) {
        return new GraphDiscoveryResultReadinessCronTask(
                runDelay,
                timeout,
                db,
                graphDiscoveryService,
                bazingaTaskManager,
                clock,
                curator
        );
    }

    @Bean
    public AutoReleaseCronTask autoReleaseCronTask(
            AutoReleaseService autoReleaseService,
            @Value("${ci.autoReleaseCronTask.runDelay}") Duration runDelay,
            @Value("${ci.autoReleaseCronTask.timeout}") Duration timeout,
            CuratorFramework curator
    ) {
        return new AutoReleaseCronTask(
                autoReleaseService,
                runDelay,
                timeout,
                curator
        );
    }

    @Bean
    public PostponeActionCronTask postponeActionCronTask(
            PostponeActionService postponeActionService,
            @Value("${ci.postponeActionCronTask.runDelay}") Duration runDelay,
            @Value("${ci.postponeActionCronTask.timeout}") Duration timeout,
            CuratorFramework curator
    ) {
        return new PostponeActionCronTask(
                postponeActionService,
                runDelay,
                timeout,
                curator
        );
    }
}
