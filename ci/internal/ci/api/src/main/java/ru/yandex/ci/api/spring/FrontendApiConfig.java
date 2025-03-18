package ru.yandex.ci.api.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.scheduling.annotation.EnableScheduling;

import ru.yandex.ci.api.controllers.frontend.FlowController;
import ru.yandex.ci.api.controllers.frontend.FlowLaunchApiService;
import ru.yandex.ci.api.controllers.frontend.JobLaunchService;
import ru.yandex.ci.api.controllers.frontend.OnCommitFlowController;
import ru.yandex.ci.api.controllers.frontend.ProjectController;
import ru.yandex.ci.api.controllers.frontend.ReleaseController;
import ru.yandex.ci.api.controllers.frontend.TimelineController;
import ru.yandex.ci.api.controllers.internal.InternalApiController;
import ru.yandex.ci.api.controllers.storage.ConfigBundleCollectionService;
import ru.yandex.ci.api.controllers.storage.StorageFlowController;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.logging.TemporalLogService;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranchCache;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.spring.DiscoveryConfig;
import ru.yandex.ci.engine.spring.PermissionsConfig;
import ru.yandex.ci.engine.spring.TrackerJobs;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;
import ru.yandex.ci.flow.spring.temporal.CiTemporalServiceConfig;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;

@Configuration
@EnableScheduling
@Import({
        DiscoveryConfig.class,
        S3LogStorageConfig.class,
        CiTemporalServiceConfig.class,
        PermissionsConfig.class,
        TrackerJobs.class
})
public class FrontendApiConfig {

    @Bean
    public InternalApiController internalApiController(
            JobProgressService jobProgressService,
            CommitFetchService commitFetchService,
            UrlService urlService,
            CiDb db
    ) {
        return new InternalApiController(jobProgressService, commitFetchService, urlService, db);
    }

    @Bean
    public ReleaseController releaseController(
            LaunchService launchService,
            PermissionsService permissionsService,
            CiDb db,
            FlowLaunchApiService flowLaunchApiService,
            AbcService abcService,
            AutoReleaseService autoReleaseService,
            BranchService branchService,
            TimelineService timelineService,
            CommitRangeService commitRangeService,
            CommitFetchService commitFetchService
    ) {
        return new ReleaseController(
                launchService,
                permissionsService,
                db,
                flowLaunchApiService,
                abcService,
                autoReleaseService,
                branchService,
                timelineService,
                commitRangeService,
                commitFetchService);
    }

    @Bean
    public ProjectController projectController(
            AbcService abcService,
            ArcService arcService,
            CiDb db,
            AutoReleaseService autoReleaseService,
            BranchService branchService,
            XivaNotifier xivaNotifier
    ) {
        return new ProjectController(
                abcService,
                arcService,
                db,
                autoReleaseService,
                branchService,
                xivaNotifier
        );
    }

    @Bean
    public JobLaunchService jobLaunchService(
            BazingaStorage bazingaStorage,
            ResourceService resourceService,
            CiDb db,
            TemporalService temporalService
    ) {
        return new JobLaunchService(bazingaStorage, resourceService, db, temporalService);
    }

    @Bean
    public FlowLaunchApiService flowLaunchApiService(
            FlowStateService flowStateService,
            FlowStateRevisionService flowStateRevisionService,
            JobInterruptionService jobInterruptionService,
            PermissionsService permissionsService,
            CiDb db
    ) {
        return new FlowLaunchApiService(
                flowStateService,
                flowStateRevisionService,
                jobInterruptionService,
                permissionsService,
                db
        );
    }

    @Bean
    public FlowController flowController(
            FlowLaunchApiService flowLaunchApiService,
            JobLaunchService jobLaunchService,
            S3LogStorage s3LogStorage,
            TemporalLogService temporalLogService,
            @Value("${ci.flowController.httpPort}") int httpPort
    ) {
        return new FlowController(flowLaunchApiService, jobLaunchService, s3LogStorage, temporalLogService, httpPort);
    }

    @Bean
    public OnCommitFlowController onCommitFlowLaunchController(
            CiDb db,
            FlowLaunchApiService flowLaunchApiService,
            PermissionsService permissionsService,
            ArcService arcService,
            ArcBranchCache arcBranchCache,
            OnCommitLaunchService onCommitLaunchService,
            CommitFetchService commitFetchService,
            FlowVarsService flowVarsService,
            LaunchService launchService
    ) {
        return new OnCommitFlowController(db,
                flowLaunchApiService,
                permissionsService,
                arcService,
                arcBranchCache,
                onCommitLaunchService,
                commitFetchService,
                flowVarsService,
                launchService
        );
    }

    @Bean
    public TimelineController timelineController(
            ArcService arcService,
            TimelineService timelineService,
            RevisionNumberService revisionNumberService,
            BranchService branchService,
            PermissionsService permissionsService,
            CiDb db,
            XivaNotifier xivaNotifier) {
        return new TimelineController(
                arcService,
                timelineService,
                branchService,
                permissionsService,
                revisionNumberService,
                db,
                xivaNotifier);
    }

    @Bean
    public ArcBranchCache arcBranchCache(ArcService arcService) {
        return new ArcBranchCache(arcService);
    }

    @Bean
    public ConfigBundleCollectionService configBundleCollectionService(
            ConfigurationService configurationService,
            CiDb db
    ) {
        return new ConfigBundleCollectionService(configurationService, db);
    }

    @Bean
    public StorageFlowController storageFlowController(
            OnCommitFlowController commitFlowController,
            ConfigurationService configurationService,
            CiDb db,
            LaunchService launchService,
            DiscoveryProgressService discoveryProgressService,
            ConfigBundleCollectionService configBundleCollectionService
    ) {
        return new StorageFlowController(
                commitFlowController,
                configurationService,
                db,
                UserUtils.loginForInternalCiProcesses(),
                launchService,
                discoveryProgressService,
                configBundleCollectionService
        );
    }

}
