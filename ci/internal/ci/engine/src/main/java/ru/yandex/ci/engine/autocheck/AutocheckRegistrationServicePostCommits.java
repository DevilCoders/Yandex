package ru.yandex.ci.engine.autocheck;

import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.autocheck.AutocheckConstants.PostCommits;
import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;
import ru.yandex.ci.engine.autocheck.model.CheckRegistrationParams;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;


@Slf4j
public class AutocheckRegistrationServicePostCommits extends AutocheckRegistrationService {

    public AutocheckRegistrationServicePostCommits(
            @Nonnull ArcService arcService,
            @Nonnull StorageApiClient storageApiClient,
            @Nonnull ArcanumClientImpl arcanumClient
    ) {
        super(arcService, storageApiClient, arcanumClient);
    }

    @Override
    List<String> getCheckRegistrationTags(CheckRegistrationParams params) {
        return List.of(
                params.getLaunchParams().getLaunchId(),
                String.valueOf(params.getLaunchParams().getRightRevision().getNumber()),
                "postcommit"
        );
    }

    @Override
    protected AutocheckJobs registerAutocheckJob(
            CheckRegistrationParams params,
            CheckOuterClass.Check check,
            CheckIteration.Iteration iteration,
            AutocheckConfiguration configuration,
            Set<String> targets
    ) {
        log.info(
                "Registering autocheck job. Check: {}, iteration: {}, configuration: {}, targets: {}.",
                check.getId(), iteration.getId(), configuration, targets
        );

        var autocheckJobs = new AutocheckJobs();

        var testenvTestName = PostCommits.getTestenvTestNameByConfigurationId(configuration.getId());

        registerRightJob(params, check, iteration, configuration, targets, autocheckJobs, testenvTestName);
        registerLeftJob(params, check, iteration, configuration, targets, autocheckJobs, testenvTestName);

        log.info("Autocheck job registered for iteration {}", iteration.getId());
        return autocheckJobs;
    }

    private void registerRightJob(CheckRegistrationParams params,
                                  CheckOuterClass.Check check,
                                  CheckIteration.Iteration iteration,
                                  AutocheckConfiguration configuration,
                                  Set<String> targets,
                                  AutocheckJobs autocheckJobs,
                                  String testenvTestName) {
        if (!configuration.hasRightTask()) {
            log.warn("Configuration {} does not have right task", configuration.getId());
            return;
        }

        int configSvnRevision = (int) arcService.getCommit(params.getAutocheckConfigRightRevision())
                .getSvnRevision();

        var config = AutocheckProtoMappers.toProtoAutocheckConfiguration(
                configuration.getRightConfig(), params.getAutocheckConfigRightRevision(), configSvnRevision
        );

        var storageTask = registerStorageTask(check, iteration, configuration.getRightConfig(), true, testenvTestName,
                null, null);

        var autocheckJobBuilder = Autocheck.AutocheckJob.newBuilder()
                .setConfiguration(config)
                .addAllTargets(targets)
                .setCustomTargetsList("") //  postcommit sb task has empty string here and not "[]"
                .setStorageTask(storageTask)
                .setSide(Autocheck.AutocheckJob.Side.RIGHT);

        var autocheckJob = autocheckJobBuilder.build();
        autocheckJobs.getRightJobs().add(autocheckJob);
        log.info("Right autocheck job registered {} for iteration {}", autocheckJob, iteration.getId());
    }

    private void registerLeftJob(CheckRegistrationParams params,
                                 CheckOuterClass.Check check,
                                 CheckIteration.Iteration iteration,
                                 AutocheckConfiguration configuration,
                                 Set<String> targets,
                                 AutocheckJobs autocheckJobs,
                                 String testenvTestName) {
        if (!configuration.hasLeftTask()) {
            log.warn("Configuration {} does not have left task", configuration.getId());
            return;
        }

        int svnRevision = (int) arcService.getCommit(params.getAutocheckConfigLeftRevision())
                .getSvnRevision();

        var config = AutocheckProtoMappers.toProtoAutocheckConfiguration(
                configuration.getLeftConfig(),
                params.getAutocheckConfigLeftRevision(),
                svnRevision
        );

        var storageTask = registerStorageTask(check, iteration, configuration.getLeftConfig(), false, testenvTestName,
                null, null);

        var autocheckJob = Autocheck.AutocheckJob.newBuilder()
                .setConfiguration(config)
                .addAllTargets(targets)
                .setStorageTask(storageTask)
                .setSide(Autocheck.AutocheckJob.Side.LEFT)
                .build();

        autocheckJobs.getLeftJobs().add(autocheckJob);
        log.info("Left autocheck job registered {} for iteration {}", autocheckJob, iteration.getId());
    }

    @Override
    protected String createJobName(AutocheckConfigurationConfig autocheckConfiguration) {
        return PostCommits.getTestenvTestNameByConfigurationId(autocheckConfiguration.getId());
    }

}
