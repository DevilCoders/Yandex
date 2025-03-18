package ru.yandex.ci.engine.spring;

import java.time.Clock;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.spring.AYamlServiceConfig;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.engine.branch.BranchNameGenerator;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.branch.BranchTraverseService;
import ru.yandex.ci.engine.branch.DefaultBranchNameGenerator;
import ru.yandex.ci.engine.config.BranchYamlService;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.flow.SecurityStateService;
import ru.yandex.ci.engine.launch.version.BranchVersionService;
import ru.yandex.ci.engine.launch.version.VersionSlotService;
import ru.yandex.ci.engine.registry.TaskRegistry;
import ru.yandex.ci.engine.registry.TaskRegistryImpl;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import({
        ArcClientConfig.class,
        YdbCiConfig.class,
        SecurityServiceConfig.class,
        TaskValidatorConfig.class,
        AYamlServiceConfig.class
})
public class ConfigurationServiceConfig {

    @Bean
    public CommitFetchService commitFetchService(
            CiMainDb db,
            ArcService arcService,
            BranchService branchService,
            TimelineService timelineService
    ) {
        return new CommitFetchService(db, arcService, branchService, timelineService);
    }

    @Bean
    public TimelineService timelineService(
            CiMainDb db,
            RevisionNumberService revisionNumberService,
            BranchTraverseService branchTraverseService) {
        return new TimelineService(db, revisionNumberService, branchTraverseService);
    }

    @Bean
    public CommitRangeService commitRangeService(
            CiMainDb db,
            TimelineService timelineService,
            BranchTraverseService branchTraverseService
    ) {
        return new CommitRangeService(db, timelineService, branchTraverseService);
    }

    @Bean
    public RevisionNumberService revisionNumberService(ArcService arcService, CiMainDb db) {
        return new RevisionNumberService(arcService, db);
    }

    @Bean
    public BranchNameGenerator branchNameGenerator() {
        return new DefaultBranchNameGenerator();
    }

    @Bean
    public BranchService branchService(
            Clock clock,
            ArcService arcService,
            CiMainDb db,
            TimelineService timelineService,
            ConfigurationService configurationService,
            BranchVersionService branchVersionService,
            SecurityAccessService securityAccessService,
            CommitRangeService commitRangeService,
            BranchNameGenerator branchNameGenerator
    ) {
        return new BranchService(clock,
                arcService,
                db,
                timelineService,
                configurationService,
                branchVersionService,
                securityAccessService,
                commitRangeService,
                branchNameGenerator
        );
    }

    @Bean
    public VersionSlotService versionSlotService(CiMainDb db) {
        return new VersionSlotService(db);
    }

    @Bean
    public BranchVersionService branchVersionService(
            CiMainDb db,
            VersionSlotService versionSlotService
    ) {
        return new BranchVersionService(db, versionSlotService);
    }

    @Bean
    public BranchTraverseService branchTraverseService(CiMainDb db) {
        return new BranchTraverseService(db);
    }

    @Bean
    public AffectedAYamlsFinder affectedAYamlsFinder(ArcService arcService) {
        return new AffectedAYamlsFinder(arcService);
    }

    @Bean
    public BranchYamlService branchYamlService(
            ArcService arcService,
            ConfigurationService configurationService) {
        return new BranchYamlService(arcService, configurationService);
    }

    @Bean
    public ConfigParseService configParseService(
            ArcService arcService,
            AYamlService aYamlService,
            TaskRegistry taskRegistry,
            InputOutputTaskValidator taskValidator,
            AbcService abcService,
            @Value("${ci.configParseService.alwaysValidateAbcSlug:false}") boolean alwaysValidateAbcSlug) {
        return new ConfigParseService(arcService, aYamlService, taskRegistry, taskValidator, abcService,
                alwaysValidateAbcSlug);
    }

    @Bean
    public ConfigurationService configurationService(
            SecurityStateService securityStateService,
            ArcService arcService,
            ConfigParseService configParseService,
            ArcanumClientImpl arcanumClient,
            RevisionNumberService revisionNumberService,
            BranchTraverseService branchTraverseService,
            CiMainDb db,
            Clock clock
    ) {
        return new ConfigurationService(
                securityStateService,
                arcService,
                configParseService,
                arcanumClient,
                revisionNumberService,
                branchTraverseService,
                db,
                clock
        );
    }

    @Bean
    public TaskRegistry taskRegistry(ArcService arcService,
                                     @Nullable MeterRegistry meterRegistry) {
        return new TaskRegistryImpl(arcService, meterRegistry);
    }
}
