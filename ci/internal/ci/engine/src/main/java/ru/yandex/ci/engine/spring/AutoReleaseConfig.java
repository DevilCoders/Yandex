package ru.yandex.ci.engine.spring;

import java.time.Clock;
import java.util.Set;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseQueue;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseSettingsService;
import ru.yandex.ci.engine.launch.auto.DelayedAutoReleaseLaunchTask;
import ru.yandex.ci.engine.launch.auto.PostponeActionService;
import ru.yandex.ci.engine.launch.auto.ReleaseScheduler;
import ru.yandex.ci.engine.launch.auto.RuleEngine;
import ru.yandex.ci.engine.launch.auto.ScheduleCalculator;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        YdbCiConfig.class,
        ConfigurationServiceConfig.class,
        LaunchConfig.class,
        FlowExecutionConfig.class
})
public class AutoReleaseConfig {

    @Bean
    public ReleaseScheduler releaseScheduler(
            BazingaTaskManager bazingaTaskManager
    ) {
        return new ReleaseScheduler(bazingaTaskManager);
    }

    @Bean
    public AutoReleaseQueue autoReleaseQueue(
            CiDb db,
            AutoReleaseSettingsService autoReleaseSettingsService,
            ConfigurationService configurationService,
            @Value("${ci.autoReleaseQueue.whiteListOfProcessIds}") String[] whiteListOfProcessIds
    ) {
        return new AutoReleaseQueue(
                db, autoReleaseSettingsService, configurationService, Set.of(whiteListOfProcessIds)
        );
    }

    @Bean
    public AutoReleaseService autoReleaseService(
            CiDb db,
            ConfigurationService configurationService,
            LaunchService launchService,
            AutoReleaseSettingsService autoReleaseSettingsService,
            RuleEngine ruleEngine,
            ReleaseScheduler releaseScheduler,
            AutoReleaseQueue autoReleaseQueue,
            MeterRegistry meterRegistry
    ) {
        return new AutoReleaseService(
                db,
                configurationService,
                launchService,
                autoReleaseSettingsService,
                ruleEngine,
                releaseScheduler,
                autoReleaseQueue,
                meterRegistry,
                true
        );
    }

    @Bean
    public PostponeActionService postponeActionService(CiDb db, LaunchService launchService) {
        return new PostponeActionService(db, launchService);
    }

    @Bean
    public AutoReleaseSettingsService autoReleaseSettingsService(CiMainDb db, Clock clock) {
        return new AutoReleaseSettingsService(db, clock);
    }

    @Bean
    public ScheduleCalculator scheduleCalculator(WorkCalendarProvider calendarProvider) {
        return new ScheduleCalculator(calendarProvider);
    }

    @Bean
    public RuleEngine ruleEngine(
            CommitRangeService commitRangeService,
            Clock clock,
            CiMainDb db,
            ScheduleCalculator scheduleCalculator
    ) {
        return new RuleEngine(commitRangeService, clock, db, scheduleCalculator);
    }

    @Bean
    public DelayedAutoReleaseLaunchTask delayedAutoReleaseLaunchTask(AutoReleaseService autoReleaseService) {
        return new DelayedAutoReleaseLaunchTask(autoReleaseService);
    }

}
