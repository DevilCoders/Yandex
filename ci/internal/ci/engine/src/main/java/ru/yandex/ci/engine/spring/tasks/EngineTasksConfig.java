package ru.yandex.ci.engine.spring.tasks;

import java.time.Clock;
import java.time.Duration;

import javax.annotation.Nullable;

import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.pciexpress.PciExpressClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.abc.AbcFavoriteProjectsService;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceImpl;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.spring.PciExpressConfig;
import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.engine.abc.AbcRefreshCronTask;
import ru.yandex.ci.engine.abc.FavoriteProjectsCronTask;
import ru.yandex.ci.engine.autocheck.ConfigurationStatusesSyncCronTask;
import ru.yandex.ci.engine.autocheck.PCMUpdateCronTask;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.config.process.CheckMinCommitsZeroCronTask;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequests;
import ru.yandex.ci.engine.discovery.arc_reflog.ProcessArcReflogRecordTask;
import ru.yandex.ci.engine.discovery.arc_reflog.ReflogProcessorService;
import ru.yandex.ci.engine.discovery.pci_dss.PciDssDiscoveryProgressCheckerCronTask;
import ru.yandex.ci.engine.discovery.task.ProcessPostCommitTask;
import ru.yandex.ci.engine.discovery.task.PullRequestDiffSetDiscoveryTask;
import ru.yandex.ci.engine.discovery.task.PullRequestDiffSetProcessedByCiTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryRestartTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryStartTask;
import ru.yandex.ci.engine.discovery.tier0.ProcessPciDssCommitTask;
import ru.yandex.ci.engine.discovery.tracker_watcher.TrackerWatchCronTask;
import ru.yandex.ci.engine.discovery.tracker_watcher.TrackerWatcher;
import ru.yandex.ci.engine.event.CiEventActivityImpl;
import ru.yandex.ci.engine.event.LaunchEventTask;
import ru.yandex.ci.engine.launch.FlowLaunchService;
import ru.yandex.ci.engine.launch.LaunchCancelTask;
import ru.yandex.ci.engine.launch.LaunchCleanupTask;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchStartTask;
import ru.yandex.ci.engine.launch.LaunchStateSynchronizer;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressChecker;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressCheckerCronTask;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.launch.cleanup.PullRequestDiffSetCompleteTask;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.pr.CreatePrCommentTask;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.registry.TaskRegistry;
import ru.yandex.ci.engine.registry.TaskRegistrySnapshotCronTask;
import ru.yandex.ci.engine.spring.DiscoveryConfig;
import ru.yandex.ci.engine.spring.EngineJobs;
import ru.yandex.ci.engine.spring.LogbrokerCiEventConfig;
import ru.yandex.ci.engine.spring.TestenvDegradationConfig;
import ru.yandex.ci.engine.spring.jobs.TrackerConfig;
import ru.yandex.ci.engine.tasks.tracker.TrackerTicketCollector;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        DiscoveryConfig.class,
        TestenvDegradationConfig.class,
        LogbrokerCiEventConfig.class,
        PciExpressConfig.class,
        TrackerConfig.class,
        EngineJobs.class,
        GraphDiscoveryTaskConfig.class,
})
public class EngineTasksConfig {

    @Bean
    public ProcessPostCommitTask processPostCommitTask(DiscoveryServicePostCommits discoveryServicePostCommits) {
        return new ProcessPostCommitTask(discoveryServicePostCommits);
    }

    @Bean
    public ProcessArcReflogRecordTask processBranchCommitsTask(ReflogProcessorService reflogProcessorService) {
        return new ProcessArcReflogRecordTask(reflogProcessorService);
    }

    @Bean
    public GraphDiscoveryStartTask graphDiscoveryStartTask(GraphDiscoveryService graphDiscoveryService) {
        return new GraphDiscoveryStartTask(graphDiscoveryService);
    }

    @Bean
    public GraphDiscoveryRestartTask graphDiscoveryRestartTask(CiMainDb db,
                                                               GraphDiscoveryService graphDiscoveryService) {
        return new GraphDiscoveryRestartTask(db, graphDiscoveryService);
    }


    @Bean
    public PullRequestDiffSetDiscoveryTask pullRequestDiffSetDiscoveryTask(
            DiscoveryServicePullRequests discoveryServicePullRequests,
            CiMainDb db
    ) {
        return new PullRequestDiffSetDiscoveryTask(discoveryServicePullRequests, db);
    }

    @Bean
    public PullRequestDiffSetCompleteTask pullRequestDiffSetCompleteTask(
            BazingaTaskManager bazingaTaskManager,
            CiMainDb db,
            @Value("${ci.pullRequestDiffSetCompleteTask.rescheduleDelay}") Duration rescheduleDelay) {
        return new PullRequestDiffSetCompleteTask(bazingaTaskManager, db, rescheduleDelay);
    }

    @Bean
    public PullRequestDiffSetProcessedByCiTask pullRequestDiffSetProcessedByCiTask(
            PullRequestService pullRequestService
    ) {
        return new PullRequestDiffSetProcessedByCiTask(pullRequestService);
    }

    @Bean
    public LaunchStartTask launchStartTask(
            CiMainDb db,
            FlowLaunchService flowLaunchService,
            ConfigurationService configurationService,
            FlowLaunchMutexManager flowLaunchMutexManage
    ) {
        return new LaunchStartTask(db, flowLaunchService, configurationService, flowLaunchMutexManage);
    }

    @Bean
    public LaunchCancelTask launchCancel(
            CiMainDb db,
            FlowLaunchService flowLaunchService,
            LaunchStateSynchronizer launchStateSynchronizer,
            Clock clock,
            FlowLaunchMutexManager flowLaunchMutexManager
    ) {
        return new LaunchCancelTask(db, flowLaunchService, launchStateSynchronizer, clock, flowLaunchMutexManager);
    }


    @Bean
    public LaunchCleanupTask launchCleanup(
            CiDb db,
            BazingaTaskManager bazingaTaskManager,
            FlowLaunchService flowLaunchService,
            LaunchService launchService,
            @Value("${ci.launchCleanup.rescheduleDelay}") Duration rescheduleDelay,
            FlowLaunchMutexManager flowLaunchMutexManager
    ) {
        return new LaunchCleanupTask(
                db,
                bazingaTaskManager,
                flowLaunchService,
                launchService,
                rescheduleDelay,
                flowLaunchMutexManager
        );
    }

    @Bean
    public DiscoveryProgressCheckerCronTask discoveryProgressCheckerCronTask(
            DiscoveryProgressChecker discoveryProgressChecker,
            @Value("${ci.discoveryProgressCheckerCronTask.runDelay}") Duration runDelay,
            @Value("${ci.discoveryProgressCheckerCronTask.timeout}") Duration timeout,
            CuratorFramework curator
    ) {
        return new DiscoveryProgressCheckerCronTask(
                discoveryProgressChecker,
                runDelay,
                timeout,
                curator
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public PCMUpdateCronTask pcmUpdateCronTask(
            PCMService pcmService,
            @Value("${ci.pcmUpdateCronTask.runDelay}") Duration runDelay,
            @Value("${ci.pcmUpdateCronTask.timeout}") Duration timeout,
            CuratorFramework curator
    ) {
        return new PCMUpdateCronTask(
                pcmService,
                runDelay,
                timeout,
                curator
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public TaskRegistrySnapshotCronTask taskRegistrySnapshotCronTask(
            CiMainDb db,
            TaskRegistry taskRegistry,
            CuratorFramework curator
    ) {
        return new TaskRegistrySnapshotCronTask(db, taskRegistry, curator);
    }

    @Bean
    public CreatePrCommentTask createPrCommentTask(ArcanumClientImpl arcanumClient) {
        return new CreatePrCommentTask(arcanumClient);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ConfigurationStatusesSyncCronTask configurationStatusesSyncCronTask(
            @Nullable CuratorFramework curator,
            CiMainDb db,
            TestenvDegradationManager testenvDegradationManager) {

        return new ConfigurationStatusesSyncCronTask(
                curator,
                db,
                testenvDegradationManager
        );
    }

    @Bean
    public LaunchEventTask launchEventTask(LogbrokerWriter logbrokerLaunchEventWriter) {
        return new LaunchEventTask(logbrokerLaunchEventWriter);
    }

    @Bean
    public CiEventActivityImpl ciEventActivity(LogbrokerWriter logbrokerCiEventWriter) {
        return new CiEventActivityImpl(logbrokerCiEventWriter);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public PciDssDiscoveryProgressCheckerCronTask pciDssDiscoveryProgressCheckerCronTask(
            DiscoveryProgressChecker discoveryProgressChecker,
            @Value("${ci.pciDssDiscoveryProgressCheckerCronTask.runDelay}") Duration runDelay,
            @Value("${ci.pciDssDiscoveryProgressCheckerCronTask.timeout}") Duration timeout,
            PciExpressClient pciExpressClient,
            CiMainDb db,
            BazingaTaskManager bazingaTaskManager,
            CuratorFramework curator
    ) {
        return new PciDssDiscoveryProgressCheckerCronTask(
                discoveryProgressChecker,
                runDelay,
                timeout,
                pciExpressClient,
                db,
                bazingaTaskManager,
                curator
        );
    }

    @Bean
    public ProcessPciDssCommitTask processPciExpressCommitTask(
            PciExpressClient pciExpressClient,
            RevisionNumberService revisionNumberService,
            ArcService arcService,
            ConfigurationService configurationService,
            DiscoveryServicePostCommits discoveryServicePostCommits,
            DiscoveryProgressService discoveryProgressService
    ) {
        return new ProcessPciDssCommitTask(
                pciExpressClient,
                revisionNumberService,
                arcService,
                configurationService,
                discoveryServicePostCommits,
                discoveryProgressService
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public TrackerWatcher trackerWatcher(
            TrackerTicketCollector trackerTicketCollector,
            OnCommitLaunchService onCommitLaunchService,
            ConfigurationService configurationService,
            FlowLaunchService flowLaunchService,
            FlowLaunchMutexManager flowLaunchMutexManager,
            ArcService arcService,
            CiDb db
    ) {
        return new TrackerWatcher(
                trackerTicketCollector,
                onCommitLaunchService,
                configurationService,
                flowLaunchService,
                flowLaunchMutexManager,
                arcService,
                db
        );
    }

    @Bean
    @Profile(CiProfile.STABLE_PROFILE)
    public TrackerWatchCronTask trackerWatchCronTask(
            TrackerWatcher trackerWatcher,
            @Value("${ci.trackerWatchCronTask.runDelay}") Duration runDelay,
            @Value("${ci.trackerWatchCronTask.timeout}") Duration timeout,
            CuratorFramework curator
    ) {
        return new TrackerWatchCronTask(
                trackerWatcher,
                runDelay,
                timeout,
                curator
        );
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public CheckMinCommitsZeroCronTask checkMinCommitsZeroCronTask(
            CiDb db,
            ConfigParseService configParseService,
            ArcService arcService,
            @Nullable CuratorFramework curator
    ) {
        return new CheckMinCommitsZeroCronTask(db, configParseService, arcService, curator);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public AbcRefreshCronTask abcRefreshCronTask(
            AbcService abcService,
            @Value("${ci.abcRefreshCronTask.runDelay}") Duration runDelay,
            @Value("${ci.abcRefreshCronTask.timeout}") Duration timeout,
            @Nullable CuratorFramework curator
    ) {
        return new AbcRefreshCronTask((AbcServiceImpl) abcService, runDelay, timeout, curator);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public FavoriteProjectsCronTask favoriteProjectsCronTask(
            AbcFavoriteProjectsService abcFavoriteProjectsService,
            @Value("${ci.favoriteProjectsCronTask.runDelay}") Duration runDelay,
            @Value("${ci.favoriteProjectsCronTask.timeout}") Duration timeout,
            @Nullable CuratorFramework curator
    ) {
        return new FavoriteProjectsCronTask(abcFavoriteProjectsService, runDelay, timeout, curator);
    }
}
