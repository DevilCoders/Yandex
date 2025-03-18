package ru.yandex.ci.engine.autocheck;

import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.config.BranchConfigBundle;
import ru.yandex.ci.engine.config.BranchYamlService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.PullRequestDiscoveryContext;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.pr.PullRequestService;

import static ru.yandex.arcanum.event.ArcanumModels.DiffSet.Type.DIFF;
import static ru.yandex.ci.client.arcanum.ArcanumClient.Require.required;

@Slf4j
@RequiredArgsConstructor
public class AutocheckBootstrapServicePullRequests {

    public static final CiProcessId BRANCH_PRECOMMIT_PROCESS_ID = CiProcessId.ofFlow(
            AutocheckConstants.AUTOCHECK_A_YAML_PATH, "autocheck-branch-precommits"
    );
    public static final CiProcessId TRUNK_PRECOMMIT_PROCESS_ID = CiProcessId.ofFlow(
            AutocheckConstants.AUTOCHECK_A_YAML_PATH, "autocheck-trunk-precommits"
    );

    @Nonnull
    private final ConfigurationService configurationService;

    @Nonnull
    private final LaunchService launchService;

    @Nonnull
    private final PullRequestService pullRequestService;

    @Nonnull
    private final BranchYamlService branchYamlService;

    @Nonnull
    private final CiMainDb db;

    @Nonnull
    private final AutocheckBlacklistService autocheckBlacklistService;

    @Nonnull
    private final ArcService arcService;

    public void runAutocheckIfRequired(PullRequestDiscoveryContext pullRequestDiscoveryContext) {
        var diffSet = pullRequestDiscoveryContext.getDiffSet();
        log.info("Checking if autocheck is enabled for pull request {}", diffSet.getId());

        var autocheckParameters = new AutocheckConditions(pullRequestDiscoveryContext).computeParameters();
        if (autocheckParameters.isPrecommitCheck()) {
            var context = createContext(pullRequestDiscoveryContext, TRUNK_PRECOMMIT_PROCESS_ID, false);
            if (context == null) {
                return;
            }
            new TrunkAutocheckBootstrapper(context).scheduleAutocheck();
            return;
        } else if (autocheckParameters.isBranchCheck()) {
            var context = createContext(pullRequestDiscoveryContext, BRANCH_PRECOMMIT_PROCESS_ID, true);
            if (context == null) {
                return;
            }
            new BranchAutocheckBootstrapper(
                    new BranchAutocheckContext(
                            context,
                            Objects.requireNonNull(autocheckParameters.getBranchConfigBundle())
                    )
            ).scheduleAutocheck();
            return;
        }

        pullRequestService.skipArcanumDefaultChecks(diffSet);
    }

    @RequiredArgsConstructor
    class AutocheckConditions {

        @Nonnull
        PullRequestDiscoveryContext pullRequestDiscoveryContext;

        AutocheckParameters computeParameters() {
            var upstreamBranch = pullRequestDiscoveryContext.getDiffSet().getVcsInfo().getUpstreamBranch();

            switch (upstreamBranch.getType()) {
                case TRUNK -> {
                    log.info("Precommit autocheck for {} will be started", upstreamBranch);
                    return AutocheckParameters.preCommitCheck();
                }

                case RELEASE_BRANCH -> {
                    var branchConfigOpt = branchYamlService.findBranchConfigWithAutocheckSection(upstreamBranch);
                    if (branchConfigOpt.isEmpty()) {
                        log.info("Skip autocheck from CI, no config for branch {}, skipping", upstreamBranch);
                        return AutocheckParameters.empty();
                    }
                    log.info("Autocheck is enabled for branch {}", upstreamBranch);
                    return AutocheckParameters.branchCheck(branchConfigOpt.get());
                }

                default -> {
                    log.info("Skip autocheck from CI, unsupported upstream branch: {}", upstreamBranch);
                    return AutocheckParameters.empty();
                }
            }
        }
    }

    @RequiredArgsConstructor
    class BranchAutocheckBootstrapper {
        @Nonnull
        private final BranchAutocheckContext context;

        void scheduleAutocheck() {
            var diffSet = context.getDiffSet();
            log.info("Scheduling autocheck from CI for {}", diffSet);

            var branchConfig = context.getBranchConfigBundle();
            if (!branchConfig.isValid()) {
                unableToStart(String.format(
                        "Skip autocheck from CI, branch config %s is invalid. Problems: %s",
                        branchConfig.getPath(),
                        branchConfig.getProblems()
                ));
                return;
            }

            startPrFlow(context.getAutocheckContext(), diffSet, context.getProcessId());
        }

        private void unableToStart(String error) {
            log.info(error);
            pullRequestService.sendAutocheckStartupFailure(context.getDiffSet().getPullRequestId(), error);
            sendArcanumStatus(context.getAutocheckContext(), ArcanumMergeRequirementDto.Status.FAILURE);
        }
    }

    @RequiredArgsConstructor
    class TrunkAutocheckBootstrapper {

        private final AutocheckContext context;

        void scheduleAutocheck() {
            var diffSet = context.getPullRequestDiscoveryContext().getDiffSet();

            var mergeRevision = diffSet.getOrderedMergeRevision();
            if (
                    diffSet.getType() != DIFF ||
                            autocheckBlacklistService.isOnlyBlacklistPathsAffected(mergeRevision.toRevision())
            ) {
                log.info("Precommit autocheck for {} is disabled (by blacklist)", diffSet.getId());
                pullRequestService.skipArcanumDefaultChecks(diffSet);
                return;
            }

            log.info("Scheduling {} for {}", context.getProcessId(), diffSet.getId());
            startPrFlow(context, diffSet, context.getProcessId());
        }

    }

    private void startPrFlow(AutocheckContext context,
                             PullRequestDiffSet diffSet,
                             CiProcessId processId) {
        var mergeRevision = diffSet.getOrderedMergeRevision();
        sendArcanumStatus(context, ArcanumMergeRequirementDto.Status.PENDING);
        // Mark commit as discovered, it's required for launching the flow
        db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                processId,
                mergeRevision,
                optionalState -> optionalState.orElseGet(
                        () -> DiscoveredCommitState.builder()
                                .manualDiscovery(true) // It's not really manual, but keep it that way for now
                                .build()
                )
        ));
        var configBundle = context.getAutocheckAYamlConfig();
        var launchMode = configBundle.isReadyForLaunch()
                ? LaunchMode.NORMAL
                : LaunchMode.DELAY;
        var launch = launchService.startPrFlow(
                processId,
                mergeRevision,
                diffSet.getAuthor(),
                configBundle,
                context.getPullRequestInfo(),
                launchMode,
                true,
                null
        );

        context.getPullRequestDiscoveryContext().addCreatedLaunch(launch.getLaunchId());
        log.info("Autocheck Flow started: {}", launch);
    }

    private void sendArcanumStatus(AutocheckContext context, ArcanumMergeRequirementDto.Status status) {
        pullRequestService.sendRequirementAndMergeStatus(
                context.getPullRequestInfo(),
                status,
                null,
                null,
                required(),
                context.getPullRequestDiscoveryContext(),
                null
        );
    }

    @Nullable
    private AutocheckContext createContext(
            PullRequestDiscoveryContext pullRequestDiscoveryContext,
            CiProcessId processId,
            boolean forceTrunkConfig
    ) {
        var autocheckAYamlConfig = getAutocheckAYamlBundle(
                forceTrunkConfig,
                pullRequestDiscoveryContext.getDiffSet().getOrderedMergeRevision()
        );
        if (autocheckAYamlConfig.getStatus() == ConfigStatus.INVALID) {
            log.warn("Autocheck config is invalid: {}", autocheckAYamlConfig.getConfigEntity().getProblems());
            //If autocheck/a.yaml is invalid in precommit just skip the launch. CI validation will do the rest.
            return null;
        }

        var ciConfig = autocheckAYamlConfig.getValidAYamlConfig().getCi();
        var title = pullRequestService.getPullRequestTitle(processId, ciConfig);

        var pullRequestInfo = pullRequestService.toLaunchPullRequestInfo(
                pullRequestDiscoveryContext.getDiffSet(), processId, title);

        return new AutocheckContext(
                processId,
                autocheckAYamlConfig,
                pullRequestInfo,
                pullRequestDiscoveryContext
        );
    }

    private ConfigBundle getAutocheckAYamlBundle(boolean forceTrunkConfig, OrderedArcRevision pullRequestRevision) {
        if (!forceTrunkConfig) {
            var lastCommit = arcService.getLastCommit(AutocheckConstants.AUTOCHECK_A_YAML_PATH, pullRequestRevision);
            if (lastCommit.isPresent() && !lastCommit.get().isTrunk()) {
                log.info("Using {} cached is commit {}", AutocheckConstants.AUTOCHECK_A_YAML_PATH, lastCommit.get());
                return configurationService.getOrCreateConfig(
                        AutocheckConstants.AUTOCHECK_A_YAML_PATH,
                        pullRequestRevision
                ).orElseThrow(() -> new RuntimeException(
                        "Unable to find configuration for " + AutocheckConstants.AUTOCHECK_A_YAML_PATH));
            }
        }

        return configurationService.getLastValidConfig(AutocheckConstants.AUTOCHECK_A_YAML_PATH, ArcBranch.trunk());
    }

    @Value
    private static class AutocheckContext {
        CiProcessId processId;
        ConfigBundle autocheckAYamlConfig;
        LaunchPullRequestInfo pullRequestInfo;
        PullRequestDiscoveryContext pullRequestDiscoveryContext;
    }

    @Value
    @AllArgsConstructor
    private static class BranchAutocheckContext {
        AutocheckContext autocheckContext;
        BranchConfigBundle branchConfigBundle;

        PullRequestDiffSet getDiffSet() {
            return autocheckContext.getPullRequestDiscoveryContext().getDiffSet();
        }

        CiProcessId getProcessId() {
            return autocheckContext.getProcessId();
        }
    }

    @Value
    @Builder
    private static class AutocheckParameters {
        boolean branchCheck;
        @Nullable
        BranchConfigBundle branchConfigBundle;

        boolean precommitCheck;

        static AutocheckParameters empty() {
            return AutocheckParameters.builder().build();
        }

        static AutocheckParameters branchCheck(@Nonnull BranchConfigBundle configBundle) {
            return AutocheckParameters.builder()
                    .branchCheck(true)
                    .branchConfigBundle(configBundle)
                    .build();
        }

        static AutocheckParameters preCommitCheck() {
            return AutocheckParameters.builder().precommitCheck(true).build();
        }
    }
}
