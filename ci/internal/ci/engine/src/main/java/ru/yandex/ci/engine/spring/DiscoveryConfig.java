package ru.yandex.ci.engine.spring;

import java.time.Clock;
import java.time.Duration;
import java.util.Set;

import io.micrometer.core.instrument.MeterRegistry;
import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.time.DurationParser;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.DiscoveryServiceFilters;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommitTriggers;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.DiscoveryServiceProcessor;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequests;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequestsTriggers;
import ru.yandex.ci.engine.discovery.PullRequestObsoleteMergeRequirementsCleaner;
import ru.yandex.ci.engine.discovery.arc_reflog.ReflogProcessorService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryServiceProperties;
import ru.yandex.ci.engine.flow.SecurityStateService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.auto.ActionPostCommitCronTask;
import ru.yandex.ci.engine.launch.auto.ActionPostCommitHandler;
import ru.yandex.ci.engine.launch.auto.AutoReleaseMetrics;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.BinarySearchExecutor;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressChecker;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.launch.auto.LargePostCommitCronTask;
import ru.yandex.ci.engine.launch.auto.LargePostCommitHandler;
import ru.yandex.ci.engine.launch.auto.LargePostCommitWriterTask;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        AutocheckBootstrapConfig.class,
        AutocheckConfig.class,
        AutoReleaseConfig.class
})
public class DiscoveryConfig {

    @Bean
    public DiscoveryServiceFilters discoveryServiceFilters(AbcService abcService) {
        return new DiscoveryServiceFilters(abcService);
    }

    @Bean
    public DiscoveryServicePostCommitTriggers discoveryServicePostCommitTriggers(DiscoveryServiceFilters filters) {
        return new DiscoveryServicePostCommitTriggers(filters);
    }

    @Bean
    public DiscoveryServicePullRequestsTriggers discoveryServicePullRequestsTriggers(DiscoveryServiceFilters filters) {
        return new DiscoveryServicePullRequestsTriggers(filters);
    }

    @Bean
    public PullRequestObsoleteMergeRequirementsCleaner pullRequestObsoleteMergeRequirementsCleaner(
            PullRequestService pullRequestService
    ) {
        return new PullRequestObsoleteMergeRequirementsCleaner(pullRequestService);
    }

    @Bean
    public OnCommitLaunchService onCommitLaunchService(
            CiMainDb db,
            ConfigurationService configurationService,
            PullRequestService pullRequestService,
            LaunchService launchService,
            RevisionNumberService revisionNumberService,
            ArcService arcService,
            Clock clock,
            @Value("${ci.onCommitLaunchService.whiteListProjects}") String[] whiteListProjects
    ) {
        return new OnCommitLaunchService(
                db,
                configurationService,
                pullRequestService,
                launchService,
                revisionNumberService,
                arcService,
                clock,
                Set.of(whiteListProjects)
        );
    }

    @Bean
    public DiscoveryServiceProcessor discoveryServiceProcessor(
            BranchService branchService,
            CiMainDb db,
            AutoReleaseService autoReleaseService,
            OnCommitLaunchService onCommitLaunchService) {
        return new DiscoveryServiceProcessor(
                branchService,
                db,
                autoReleaseService,
                onCommitLaunchService);
    }

    @Bean
    public DiscoveryServicePostCommits discoveryServicePostCommits(
            AffectedAYamlsFinder affectedAYamlsFinder,
            ArcService arcService,
            BranchService branchService,
            CiMainDb db,
            ConfigurationService configurationService,
            DiscoveryServicePostCommitTriggers triggers,
            DiscoveryServiceProcessor processor,
            AutocheckBootstrapServicePostCommits autocheckBootstrapServicePostCommits,
            DiscoveryProgressService discoveryProgressService,
            RevisionNumberService revisionNumberService,
            @Value("${ci.discoveryService.autocheckPostcommitsEnabled}") boolean autocheckPostcommitsEnabled
    ) {
        return new DiscoveryServicePostCommits(
                affectedAYamlsFinder,
                arcService,
                branchService,
                db,
                configurationService,
                triggers,
                processor,
                autocheckBootstrapServicePostCommits,
                discoveryProgressService,
                revisionNumberService,
                autocheckPostcommitsEnabled
        );
    }

    @Bean
    public DiscoveryServicePullRequests discoveryServicePullRequests(
            AffectedAYamlsFinder affectedAYamlsFinder,
            AbcService abcService,
            ArcService arcService,
            ConfigurationService configurationService,
            PullRequestService pullRequestService,
            CiMainDb db,
            LaunchService launchService,
            PullRequestObsoleteMergeRequirementsCleaner obsoleteMergeRequirementsCleaner,
            DiscoveryServicePullRequestsTriggers triggers,
            AutocheckBootstrapServicePullRequests autocheckBootstrapServicePullRequests,
            SecurityStateService securityStateService
    ) {
        return new DiscoveryServicePullRequests(
                affectedAYamlsFinder,
                abcService,
                arcService,
                configurationService,
                pullRequestService,
                db,
                launchService,
                obsoleteMergeRequirementsCleaner,
                triggers,
                autocheckBootstrapServicePullRequests,
                securityStateService
        );
    }

    @Bean
    public GraphDiscoveryService graphDiscoveryService(
            DiscoveryProgressService discoveryProgressService,
            CiMainDb db,
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager,
            Clock clock,
            GraphDiscoveryServiceProperties graphDiscoveryServiceProperties,
            AutocheckBlacklistService autocheckBlacklistService
    ) {
        return new GraphDiscoveryService(
                discoveryProgressService,
                db,
                sandboxClient,
                bazingaTaskManager,
                clock,
                graphDiscoveryServiceProperties,
                autocheckBlacklistService
        );
    }

    @Bean
    public GraphDiscoveryServiceProperties graphDiscoveryServiceProperties(
            @Value("${ci.graphDiscoveryService.enabled}") boolean enabled,
            @Value("${ci.graphDiscoveryService.sandboxTaskType}") String sandboxTaskType,
            @Value("${ci.graphDiscoveryService.sandboxTaskOwner}") String sandboxTaskOwner,
            @Value("${ci.graphDiscoveryService.useDistbuildTestingCluster}") boolean useDistbuildTestingCluster,
            @Value("${ci.graphDiscoveryService.secretWithYaToolToken}") String secretWithYaToolToken,
            @Value("${ci.graphDiscoveryService.distBuildPool}") String distBuildPool,
            @Value("${ci.graphDiscoveryService.delayBetweenSandboxTaskRestarts}")
            Duration delayBetweenSandboxTaskRestarts
    ) {
        return GraphDiscoveryServiceProperties.builder()
                .enabled(enabled)
                .sandboxTaskType(sandboxTaskType)
                .sandboxTaskOwner(sandboxTaskOwner)
                .useDistbuildTestingCluster(useDistbuildTestingCluster)
                .secretWithYaToolToken(secretWithYaToolToken)
                .distBuildPool(distBuildPool)
                .delayBetweenSandboxTaskRestarts(delayBetweenSandboxTaskRestarts)
                .build();
    }

    @Bean
    public DiscoveryProgressChecker discoveryProgressChecker(
            CiMainDb db,
            @Value("${ci.discoveryProgressChecker.lastProcessedCommitInitializationDelay}")
            Duration lastProcessedCommitInitializationDelay,
            @Value("${ci.discoveryProgressChecker.failIfUninitialized}") boolean failIfUninitialized,
            @Value("${ci.discoveryProgressChecker.maxProcessedCommitsPerRun}") int maxProcessedCommitsPerRun,
            Clock clock
    ) {
        return new DiscoveryProgressChecker(
                lastProcessedCommitInitializationDelay,
                failIfUninitialized,
                db,
                clock,
                maxProcessedCommitsPerRun
        );
    }

    @Bean
    public ReflogProcessorService reflogProcessorService(
            BazingaTaskManager bazingaTaskManager,
            DiscoveryProgressService discoveryProgressService,
            GraphDiscoveryService graphDiscoveryService,
            ArcService arcService,
            CiMainDb db
    ) {
        return new ReflogProcessorService(
                bazingaTaskManager,
                discoveryProgressService,
                graphDiscoveryService,
                arcService,
                db
        );
    }

    @Bean
    public AutoReleaseMetrics autoReleaseMetrics(
            ArcService arcService,
            CiMainDb db,
            DiscoveryProgressChecker discoveryProgressChecker,
            GraphDiscoveryService graphDiscoveryService,
            Clock clock,
            MeterRegistry meterRegistry
    ) {
        return new AutoReleaseMetrics(arcService, db, discoveryProgressChecker, graphDiscoveryService, clock,
                meterRegistry);
    }

    @Bean
    public BinarySearchExecutor binarySearchExecutor(
            DiscoveryProgressChecker discoveryProgressChecker,
            LaunchService launchService,
            CiDb db
    ) {
        return new BinarySearchExecutor(discoveryProgressChecker, launchService, db);
    }


    @Bean
    public LargePostCommitHandler largePostCommitHandler(
            StorageApiClient storageApiClient,
            BazingaTaskManager bazingaTaskManager,
            @Value("${ci.largePostCommitService.delayBetweenLaunches}") Duration delayBetweenLaunches,
            @Value("${ci.largePostCommitService.delayBetweenSlowLaunches}") Duration delayBetweenSlowLaunches,
            @Value("${ci.largePostCommitService.closeLaunchesOlderThan}") Duration closeLaunchesOlderThan,
            @Value("${ci.largePostCommitService.maxActiveLaunches}") int maxActiveLaunches
    ) {
        return new LargePostCommitHandler(
                storageApiClient,
                bazingaTaskManager,
                delayBetweenLaunches,
                delayBetweenSlowLaunches,
                closeLaunchesOlderThan,
                maxActiveLaunches
        );
    }

    @Bean
    public LargePostCommitWriterTask largePostCommitWriterTask(
            StorageApiClient storageApiClient,
            UrlService urlService,
            CiDb db,
            Clock clock
    ) {
        return new LargePostCommitWriterTask(storageApiClient, urlService, db, clock);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public LargePostCommitCronTask largePostCommitCronTask(
            BinarySearchExecutor binarySearchExecutor,
            LargePostCommitHandler largePostCommitHandler,
            @Value("${ci.largePostCommitCronTask.runDelay}") String runDelay,
            @Value("${ci.largePostCommitCronTask.timeout}") String timeout,
            CuratorFramework curator
    ) {
        return new LargePostCommitCronTask(
                binarySearchExecutor,
                largePostCommitHandler,
                DurationParser.parse(runDelay),
                DurationParser.parse(timeout),
                curator
        );
    }

    @Bean
    public ActionPostCommitHandler actionPostCommitHandler(CiDb db) {
        return new ActionPostCommitHandler(db);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ActionPostCommitCronTask actionPostCommitCronTask(
            BinarySearchExecutor binarySearchExecutor,
            ActionPostCommitHandler actionPostCommitHandler,
            @Value("${ci.actionPostCommitCronTask.runDelay}") String runDelay,
            @Value("${ci.actionPostCommitCronTask.timeout}") String timeout,
            CuratorFramework curator
    ) {
        return new ActionPostCommitCronTask(
                binarySearchExecutor,
                actionPostCommitHandler,
                DurationParser.parse(runDelay),
                DurationParser.parse(timeout),
                curator
        );
    }
}
