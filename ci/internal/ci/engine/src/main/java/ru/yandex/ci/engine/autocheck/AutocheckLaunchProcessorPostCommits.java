package ru.yandex.ci.engine.autocheck;

import java.util.Set;
import java.util.TreeSet;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.experimental.FieldDefaults;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.autocheck.AutocheckConstants.PostCommits;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.autocheck.model.CheckLaunchParams;
import ru.yandex.ci.engine.autocheck.model.CheckRegistrationParams;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Slf4j
@RequiredArgsConstructor
@FieldDefaults(makeFinal = true, level = AccessLevel.PRIVATE)
public class AutocheckLaunchProcessorPostCommits {

    @Nonnull
    AutocheckYamlService autocheckYamlService;

    @Nonnull
    AutocheckRegistrationServicePostCommits autocheckRegistrationServicePostCommits;

    public Autocheck.AutocheckLaunch create(CheckLaunchParams params) {
        Preconditions.checkArgument(!params.isPrecommitCheck(), "Only postcommit check is supported");
        var leftRevision = params.getLeftRevision();
        var rightRevision = params.getRightRevision();
        var checkId = params.getArcanumCheckId();
        var checkAuthor = params.getCheckAuthor();

        var baseBranch = leftRevision.getBranch();

        log.info(
                "Creating autocheck launch. " +
                        "Base branch {}, left revision {}, right revision {}, check author {}, CI check {}",
                baseBranch, leftRevision, rightRevision, checkAuthor, checkId
        );

        Preconditions.checkState(
                baseBranch.isTrunk(),
                "Autocheck is not supported for branch type " + baseBranch.getType()
        );

        var checkRegistrationParams = getPostcommitCheckRegistrationParams(params);
        if (checkRegistrationParams.getAutocheckConfigurations().isEmpty()) {
            log.info("Autocheck launch skipped, cause autocheckConfigurations is empty." +
                            "Base branch {}, left revision {}, right revision {}, check author {}, CI check {}",
                    baseBranch, leftRevision, rightRevision, checkAuthor, checkId);
            return Autocheck.AutocheckLaunch.newBuilder()
                    .setSkip(true)
                    .build();
        }

        return autocheckRegistrationServicePostCommits.register(checkRegistrationParams);
    }

    private CheckRegistrationParams getPostcommitCheckRegistrationParams(CheckLaunchParams params) {
        var leftConfig = autocheckYamlService.getLastConfigForRevision(params.getLeftRevision());
        var rightConfig = autocheckYamlService.getLastConfigForRevision(params.getRightRevision());

        log.info(
                "Left config revision {}, right config revision {}",
                leftConfig.getRevision(), rightConfig.getRevision()
        );

        var autocheckLaunchConfig = new AutocheckLaunchConfig(
                Set.of(),   // native builds
                Set.of(),   // large tests
                CheckOuterClass.LargeConfig.getDefaultInstance(), // No default large configuration
                AutocheckConstants.FULL_CIRCUIT_TARGETS,
                new TreeSet<>(),    // fast targets
                false,
                Set.of(),           // invalid ayamls
                PostCommits.DEFAULT_POOL,
                false,
                CheckIteration.StrongModePolicy.BY_A_YAML,
                leftConfig,
                rightConfig
        );

        var autocheckConfigurations = AutocheckLaunchProcessor.mergeConfigurations(autocheckLaunchConfig, Set.of());

        return CheckRegistrationParams.builder()
                .launchParams(params)
                .autocheckLaunchConfig(autocheckLaunchConfig)
                .autocheckConfigurations(autocheckConfigurations)
                .disabledConfigurations(Set.of())
                .distbuildPriority(
                        CheckOuterClass.DistbuildPriority.newBuilder()
                                .setPriorityRevision(params.getRightRevision().getNumber())
                                .build()
                )
                .zipatch(CheckOuterClass.Zipatch.getDefaultInstance())
                .build();
    }

}
