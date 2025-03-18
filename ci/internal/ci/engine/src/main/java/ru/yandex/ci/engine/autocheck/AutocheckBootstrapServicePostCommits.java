package ru.yandex.ci.engine.autocheck;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;

@Slf4j
@RequiredArgsConstructor
public class AutocheckBootstrapServicePostCommits {

    public static final CiProcessId TRUNK_POSTCOMMIT_PROCESS_ID = CiProcessId.ofFlow(
            AutocheckConstants.AUTOCHECK_A_YAML_PATH, "autocheck-trunk-postcommits"
    );

    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final CiMainDb db;

    public boolean runAutocheckIfRequired(ArcBranch origBranch, OrderedArcRevision revision, ArcCommit commit) {
        if (!origBranch.isTrunk()) {
            log.info("Postcommit autocheck is disabled for branch {}, skipping {}", origBranch, revision);
            return false;
        }

        new TrunkAutocheckBootstrapper(new AutocheckContext(
                origBranch,
                revision,
                commit,
                TRUNK_POSTCOMMIT_PROCESS_ID,
                getAutocheckAYamlBundle()
        )).scheduleAutocheck();
        return true;
    }

    @RequiredArgsConstructor
    class TrunkAutocheckBootstrapper {

        private final AutocheckContext context;

        void scheduleAutocheck() {
            log.info("Scheduling {} in {} at {}", context.getProcessId(), context.getOrigBranch(),
                    context.getRevision());

            // Mark commit as discovered, it's required for launching the flow
            db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                    context.getProcessId(),
                    context.getRevision(),
                    optionalState -> optionalState.orElseGet(
                            () -> DiscoveredCommitState.builder()
                                    .manualDiscovery(true) // It's not really manual, but keep it that way for now
                                    .build()
                    )
            ));

            var launchMode = context.getAutocheckAYamlConfig().isReadyForLaunch()
                    ? LaunchMode.POSTPONE
                    : LaunchMode.DELAY;
            launchService.createAndStartLaunch(LaunchService.LaunchParameters.builder()
                    .processId(context.getProcessId())
                    .launchType(Launch.Type.USER)
                    .bundle(context.getAutocheckAYamlConfig())
                    .triggeredBy(context.getCommit().getAuthor())
                    .revision(context.getRevision())
                    .selectedBranch(context.getOrigBranch())
                    .launchMode(launchMode)
                    .build()
            );
        }
    }

    private ConfigBundle getAutocheckAYamlBundle() {
        return configurationService.getLastValidConfig(AutocheckConstants.AUTOCHECK_A_YAML_PATH, ArcBranch.trunk());
    }

    @Value
    private static class AutocheckContext {
        @Nonnull
        ArcBranch origBranch;
        @Nonnull
        OrderedArcRevision revision;
        @Nonnull
        ArcCommit commit;
        @Nonnull
        CiProcessId processId;
        @Nonnull
        ConfigBundle autocheckAYamlConfig;
    }

}
