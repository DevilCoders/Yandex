package ru.yandex.ci.engine.spring;

import java.time.Clock;
import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.apache.curator.framework.CuratorFramework;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.AutocheckLaunchProcessor;
import ru.yandex.ci.engine.autocheck.AutocheckLaunchProcessorPostCommits;
import ru.yandex.ci.engine.autocheck.AutocheckRegistrationService;
import ru.yandex.ci.engine.autocheck.AutocheckRegistrationServicePostCommits;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.FastCircuitTargetsAutoResolver;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.CancelObsoleteChecksJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.MarkStorageDiscoveredJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckRecheckJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartTestenvJob;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartTestenvJobSemaphore;
import ru.yandex.ci.engine.autocheck.testenv.TestenvStartService;
import ru.yandex.ci.engine.config.BranchYamlService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.pcm.DistBuildPCMService;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.spring.clients.ArcanumClientConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientConfig;
import ru.yandex.ci.engine.spring.clients.StorageClientConfig;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.flow.spring.FlowZkConfig;
import ru.yandex.ci.flow.utils.UrlService;

@Configuration
@Import({
        CommonConfig.class,
        StorageClientConfig.class,
        ArcanumClientConfig.class,
        TestenvClientConfig.class,
        ConfigurationServiceConfig.class,
        StaffClientConfig.class,
        DiscoveryProgressConfig.class,
        AutocheckBlacklistConfig.class,
        FlowZkConfig.class
})
public class AutocheckConfig {

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public PCMService pcmService(
            Clock clock,
            @Value("${ci.pcmService.endpoint}") String endpoint,
            @Value("${ci.pcmService.deadLineAfter}") Duration deadLineAfter,
            CiMainDb db,
            MeterRegistry meterRegistry,
            @Value("${ci.pcmService.bootstrapOnStart}") boolean bootstrapOnStart,
            AbcService abcService
    ) {
        var properties = GrpcClientProperties.builder()
                .endpoint(endpoint)
                .deadlineAfter(deadLineAfter)
                .build();
        return new DistBuildPCMService(clock, properties, abcService, db, meterRegistry, bootstrapOnStart);
    }

    @Bean
    public PCMSelector pcmSelector(AbcService abcService, PCMService pcmService) {
        return new PCMSelector(abcService, pcmService);
    }

    @Bean
    public AutocheckYamlService autocheckYamlService(ArcService arcService) {
        return new AutocheckYamlService(arcService);
    }

    @Bean
    public AutocheckRegistrationService autocheckRegistrationService(
            ArcService arcService,
            StorageApiClient storageApiClient,
            ArcanumClientImpl arcanumClient
    ) {
        return new AutocheckRegistrationService(
                arcService,
                storageApiClient,
                arcanumClient
        );
    }

    @Bean
    public AutocheckRegistrationServicePostCommits autocheckRegistrationServicePostCommits(
            ArcService arcService,
            StorageApiClient storageApiClient,
            ArcanumClientImpl arcanumClient
    ) {
        return new AutocheckRegistrationServicePostCommits(arcService, storageApiClient, arcanumClient);
    }

    @Bean
    public AutocheckLaunchProcessor autocheckLaunchProcessor(
            ArcService arcService,
            AYamlService aYamlService,
            PCMSelector pcmSelector,
            AutocheckYamlService autocheckYamlService,
            BranchYamlService branchYamlService,
            ArcanumClientImpl arcanumClient,
            AutocheckRegistrationService autocheckRegistrationService,
            FastCircuitTargetsAutoResolver fastCircuitTargetsAutoResolver,
            CiMainDb db
    ) {
        return new AutocheckLaunchProcessor(
                arcService,
                aYamlService,
                pcmSelector,
                autocheckYamlService,
                branchYamlService,
                arcanumClient,
                autocheckRegistrationService,
                fastCircuitTargetsAutoResolver,
                db
        );
    }

    @Bean
    public AutocheckLaunchProcessorPostCommits autocheckLaunchProcessorPostCommits(
            AutocheckYamlService autocheckYamlService,
            AutocheckRegistrationServicePostCommits autocheckRegistrationServicePostCommits
    ) {
        return new AutocheckLaunchProcessorPostCommits(
                autocheckYamlService,
                autocheckRegistrationServicePostCommits
        );
    }

    @Bean
    public AutocheckService autocheckService(
            AutocheckLaunchProcessor autocheckLaunchProcessor,
            AutocheckLaunchProcessorPostCommits autocheckLaunchProcessorPostCommits,
            StorageApiClient storageApiClient,
            CiMainDb db,
            AutocheckBlacklistService autocheckBlacklistService,
            RevisionNumberService revisionNumberService
    ) {
        return new AutocheckService(autocheckLaunchProcessor, autocheckLaunchProcessorPostCommits, storageApiClient,
                db, autocheckBlacklistService, revisionNumberService);
    }

    @Bean
    public FastCircuitTargetsAutoResolver fastCircuitTargetsAutoResolver(
            ArcService arcService,
            CiMainDb db,
            @Value("${ci.fastCircuitTargetsAutoResolver.cacheTtl}") Duration cacheTtl
    ) {
        return new FastCircuitTargetsAutoResolver(arcService, db, cacheTtl);
    }

    @Bean
    public TestenvStartService testenvStartService(
            TestenvClient testenvClient,
            ArcanumClientImpl arcanumClient,
            ArcService arcService,
            AutocheckBlacklistService autocheckBlacklistService
    ) {
        return new TestenvStartService(testenvClient, arcanumClient, arcService, autocheckBlacklistService);
    }

    @Bean
    public CancelObsoleteChecksJob cancelObsoleteChecksJob(AutocheckService autocheckService) {
        return new CancelObsoleteChecksJob(autocheckService);
    }

    @Bean
    public StartAutocheckJob startAutocheckJob(
            AutocheckService autocheckService,
            AutocheckBlacklistService autocheckBlacklistService,
            CiMainDb db,
            UrlService urlService
    ) {
        return new StartAutocheckJob(
                autocheckService,
                autocheckBlacklistService,
                db,
                urlService
        );
    }

    @Bean
    public StartAutocheckRecheckJob startAutocheckRecheckJob(
            AutocheckService autocheckService,
            StorageApiClient storageApiClient,
            UrlService urlService
    ) {
        return new StartAutocheckRecheckJob(autocheckService, storageApiClient, urlService);
    }

    @Bean
    public StartTestenvJob startTestenvJob(
            TestenvStartService testenvStartService,
            StorageApiClient storageApiClient,
            StartTestenvJobSemaphore startTestenvJobSemaphore
    ) {
        return new StartTestenvJob(testenvStartService, storageApiClient, startTestenvJobSemaphore);
    }

    @Bean
    public StartTestenvJobSemaphore startTestenvJobSemaphore(
            CuratorFramework curatorFramework,
            CiMainDb db
    ) {
        return new StartTestenvJobSemaphore(curatorFramework, db);
    }


    @Bean
    public MarkStorageDiscoveredJob markStorageDiscoveredJob(DiscoveryProgressService discoveryProgressService) {
        return new MarkStorageDiscoveredJob(discoveryProgressService);
    }
}
