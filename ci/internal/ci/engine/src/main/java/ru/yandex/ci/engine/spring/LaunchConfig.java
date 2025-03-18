package ru.yandex.ci.engine.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchStateSynchronizer;
import ru.yandex.ci.engine.launch.version.LaunchVersionService;
import ru.yandex.ci.engine.launch.version.VersionSlotService;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        ConfigurationServiceConfig.class,
        BazingaCoreConfig.class,
        XivaNotificationConfig.class,
        CiNotificationConfig.class
})
public class LaunchConfig {

    @Bean
    public LaunchVersionService launchVersionService(
            BranchService branchService,
            CiMainDb db,
            VersionSlotService versionSlotService
    ) {
        return new LaunchVersionService(branchService, db, versionSlotService);
    }

    @Bean
    public LaunchService launchService(
            CiMainDb db,
            RevisionNumberService revisionNumberService,
            ConfigurationService configurationService,
            BazingaTaskManager bazingaTaskManager,
            BranchService branchService,
            CommitRangeService commitRangeService,
            LaunchVersionService launchVersionService,
            Clock clock,
            EventPublisher eventPublisher,
            FlowVarsService flowVarsService,
            MeterRegistry meterRegistry,
            ArcService arcService) {
        return new LaunchService(
                db,
                revisionNumberService,
                configurationService,
                bazingaTaskManager,
                branchService,
                commitRangeService,
                launchVersionService,
                clock,
                eventPublisher,
                flowVarsService,
                meterRegistry,
                arcService);
    }

    @Bean
    public LaunchStateSynchronizer flowLaunchUpdateDelegate(
            CiMainDb db,
            PullRequestService pullRequestService,
            BranchService branchService,
            EventPublisher eventPublisher,
            CommitRangeService commitRangeService,
            BazingaTaskManager bazingaTaskManager,
            Clock clock,
            XivaNotifier xivaNotifier
    ) {
        return new LaunchStateSynchronizer(db,
                pullRequestService,
                branchService,
                commitRangeService,
                eventPublisher,
                bazingaTaskManager,
                clock,
                xivaNotifier
        );
    }


}
