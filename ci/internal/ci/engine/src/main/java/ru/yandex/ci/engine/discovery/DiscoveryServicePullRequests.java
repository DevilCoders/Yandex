package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.EntryStream;

import ru.yandex.ci.client.arcanum.ArcanumClient.Require;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.AffectedAYaml;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.util.ConfigStates;
import ru.yandex.ci.engine.flow.SecurityStateService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.pr.PullRequestService;

@Slf4j
@AllArgsConstructor
public class DiscoveryServicePullRequests {

    @Nonnull
    private final AffectedAYamlsFinder affectedAYamlsFinder;
    @Nonnull
    private final AbcService abcService;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final PullRequestService pullRequestService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final PullRequestObsoleteMergeRequirementsCleaner obsoleteMergeRequirementsCleaner;
    @Nonnull
    private final DiscoveryServicePullRequestsTriggers triggers;
    @Nonnull
    private final AutocheckBootstrapServicePullRequests autocheckBootstrapServicePullRequests;
    @Nonnull
    private final SecurityStateService securityStateService;

    public List<LaunchId> processDiffSet(PullRequestDiffSet diffSet) {
        return processDiffSet(diffSet, true);
    }

    public List<LaunchId> processDiffSet(PullRequestDiffSet diffSet, boolean enableAutocheck) {
        if (diffSet.getStatus() != PullRequestDiffSet.Status.NEW) {
            log.info("Diff set {} has unexpected status: {}, skipping", diffSet.getId(), diffSet.getStatus());
            return List.of();
        }

        log.info("Discovering pr changes for PR diff set {}", diffSet);

        return db.currentOrTx(() -> {
            var prDiscoveryContext = new PullRequestDiscoveryContext(diffSet);
            if (enableAutocheck) {
                autocheckBootstrapServicePullRequests.runAutocheckIfRequired(prDiscoveryContext);
            }

            var revision = diffSet.getOrderedMergeRevision();
            var upstreamRevision = diffSet.getVcsInfo().getUpstreamRevision();
            var pullRequestId = diffSet.getPullRequestId();
            var diffSetId = diffSet.getDiffSetId();

            var affectedConfigs = affectedAYamlsFinder.getAffectedConfigs(revision.toRevision(), upstreamRevision);

            for (var affectedAYaml : affectedConfigs.getYamls()) {
                processPrConfig(diffSet, affectedAYaml, revision, upstreamRevision, prDiscoveryContext);
            }

            if (!affectedConfigs.getAYmlNotYamls().isEmpty()) {
                pullRequestService.sendAYmlNotYamlWasChangedComment(pullRequestId, affectedConfigs.getAYmlNotYamls());
            }

            obsoleteMergeRequirementsCleaner.clean(pullRequestId, diffSetId, prDiscoveryContext);

            var diffSetNew = db.pullRequestDiffSetTable().getById(pullRequestId, diffSetId);
            if (diffSetNew.getStatus() == PullRequestDiffSet.Status.NEW) {
                db.pullRequestDiffSetTable().save(diffSetNew.withStatus(PullRequestDiffSet.Status.DISCOVERED));
            } else {
                log.info("Skip updating diff-set due to status {}", diffSetNew.getState());
            }

            pullRequestService.sendProcessedByCiMergeRequirementStatus(pullRequestId, diffSetId);

            return prDiscoveryContext.getCreatedLaunches();
        });
    }

    private void processPrConfig(PullRequestDiffSet diffSet,
                                 AffectedAYaml affectedAYaml,
                                 OrderedArcRevision revision,
                                 ArcRevision upstreamRevision,
                                 PullRequestDiscoveryContext prDiscoveryContext) {
        log.info("Processing PR config {}. Config change type {}", affectedAYaml.getPath(),
                affectedAYaml.getChangeType());

        if (!affectedAYaml.getChangeType().isExisting()) {
            log.info("Config {} removed in PR {}, diffSet {}", affectedAYaml.getPath(), diffSet.getPullRequestId(),
                    diffSet.getDiffSetId());
            return;
        }

        var pullRequestConfigInfo = configurationService.getOrCreatePrConfig(affectedAYaml.getPath(), revision);
        var configBundle = pullRequestConfigInfo.getConfig();

        new PullRequestProcessor(diffSet, affectedAYaml, revision, upstreamRevision, prDiscoveryContext, configBundle)
                .process();
    }

    @AllArgsConstructor
    class PullRequestProcessor {

        @Nonnull
        private final PullRequestDiffSet diffSet;
        @Nonnull
        private final AffectedAYaml affectedAYaml;
        @Nonnull
        private final OrderedArcRevision revision;
        @Nonnull
        private final ArcRevision upstreamRevision;
        @Nonnull
        private final PullRequestDiscoveryContext prDiscoveryContext;
        @Nonnull
        private final ConfigBundle configBundle;

        void process() {
            if (configBundle.getStatus() == ConfigStatus.NOT_CI) {
                log.info("Config {} has no ci section. Ignoring...", configBundle.getConfigPath());

                if (affectedAYaml.getChangeType() != ConfigChangeType.NONE) {
                    pullRequestService.sendConfigBypassResult(diffSet.getPullRequestId(), diffSet.getDiffSetId(),
                            configBundle.getConfigEntity());
                }

                return;
            }

            if (affectedAYaml.getChangeType() != ConfigChangeType.NONE
                    || configBundle.getStatus() == ConfigStatus.INVALID) {
                sendConfigValidationResultToArcanum();
            }

            if (configBundle.getStatus() == ConfigStatus.INVALID) {
                return;
            }

            DiscoveryContext context = createContext();
            CiConfig ciConfig = configBundle.getValidAYamlConfig().getCi();

            var triggeredFlows = getActionsTriggeredOnPr(context, ciConfig.getMergedActions());
            log.info(
                    "Got {} triggered flows in config {} for PR {}, diffSet {}: {}",
                    triggeredFlows.size(),
                    configBundle.getConfigPath(),
                    diffSet.getPullRequestId(),
                    diffSet.getDiffSetId(),
                    triggeredFlows.keySet()
            );

            if (configBundle.getStatus() == ConfigStatus.SECURITY_PROBLEM) {
                notifyPullRequestIfConfigHasSecurityProblems(!triggeredFlows.isEmpty(), context);
            }

            if (triggeredFlows.isEmpty()) {
                return; // ---
            }

            LaunchMode launchMode;
            if (configBundle.getStatus() == ConfigStatus.SECURITY_PROBLEM) {
                launchMode = LaunchMode.DELAY;
                log.info("Config security state is not valid ({}), launches will be delayed for {}",
                        configBundle.getConfigEntity().getSecurityState(), configBundle.getConfigPath());
            } else {
                launchMode = LaunchMode.NORMAL;
            }

            updateConfigState(context);

            for (var triggerCfg : triggeredFlows.values()) {
                startPrFlow(ciConfig, triggerCfg, context, launchMode)
                        .ifPresent(prDiscoveryContext::addCreatedLaunch);
            }

        }

        private void updateConfigState(DiscoveryContext context) {
            var existingState = db.configStates().find(configBundle.getConfigPath());
            if (existingState.isPresent() &&
                    existingState.get().getStatus() != ConfigState.Status.DRAFT) {
                return; // Configuration is already known and not in draft
            }

            var commit = context.getCommit();

            // Do not include permissions until post-commits (keep only default)
            var state = ConfigStates.prepareConfigState(configBundle, commit)
                    .status(ConfigState.Status.DRAFT)
                    .build();

            // Store configState to make sure UI and security checks will work
            log.info("Registering config in DRAFT state: {}", state);
            db.configStates().save(state);
        }

        private Optional<LaunchId> startPrFlow(
                CiConfig ciConfig,
                TriggerCfg triggerCfg,
                DiscoveryContext context,
                LaunchMode launchMode
        ) {
            var path = context.getConfigEntity().getConfigPath();
            var action = triggerCfg.action;
            var processId = CiProcessId.ofFlow(path, action.getId());

            log.info("Starting flow {}", processId);

            var discoveredCommit = db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                    processId, context.getRevision(), context.getConfigChange(), context.getDiscoveryType()
            ));

            if (!discoveredCommit.getState().getLaunchIds().isEmpty()) {
                log.info("DiscoveredCommit {} already has launch. Skipping", discoveredCommit);
                return Optional.empty();
            }

            var title = Objects.requireNonNullElseGet(
                    action.getTitle(),
                    () -> ciConfig.getFlow(action.getFlow()).getTitle());

            var pullRequestInfo = pullRequestService.toLaunchPullRequestInfo(diffSet, processId, title);

            var require = triggerCfg.required
                    ? Require.required()
                    : Require.notRequired("Non required by a.yaml");

            var description = launchMode == LaunchMode.DELAY
                    ? "Waiting for delegation"
                    : null;
            pullRequestService.sendRequirementAndMergeStatus(
                    pullRequestInfo,
                    ArcanumMergeRequirementDto.Status.PENDING,
                    null,
                    null,
                    require,
                    prDiscoveryContext,
                    description
            );

            var launch = launchService.startPrFlow(
                    processId,
                    context.getRevision(),
                    diffSet.getAuthor(),
                    context.getConfigBundle(),
                    pullRequestInfo,
                    launchMode,
                    true,
                    null
            );

            log.info("Launch created {}", launch);

            return Optional.of(launch.getLaunchId());
        }


        private void notifyPullRequestIfConfigHasSecurityProblems(boolean hasTriggeredFlows, DiscoveryContext context) {
            var changeType = affectedAYaml.getChangeType();
            var pullRequestId = diffSet.getPullRequestId();

            if (!changeType.isExisting()) {
                log.info("skip sending security problem comment to pull request {}, cause ci section was deleted",
                        pullRequestId);
                return;
            }

            /*
                CI-4003: Условия требования делегации в PR:
                1) Поправил конфиг
                2) Владалец

                Условия требования делегации через CI Action (пока что оставляем сообщение как в п.1):
                1) Наличие hasTriggeredFlows = true

                Во всех остальных случаях делегация не нужна
             */

            if (changeType == ConfigChangeType.ADD || changeType == ConfigChangeType.MODIFY ||
                    hasTriggeredFlows || isUserBelongsToConfig(context)) {
                log.info("sending security problem comment to pull request {}, " +
                        "change type {}, has triggered flows {}", pullRequestId, changeType, hasTriggeredFlows);
                pullRequestService.sendSecurityProblemComment(pullRequestId, configBundle);
            } else {
                log.info("skip sending security problem commit to pull request {}, " +
                        "not modified, not owner and no triggered flows", pullRequestId);
            }
        }

        private boolean isUserBelongsToConfig(DiscoveryContext context) {
            return securityStateService.isUserBelongsToConfig(
                    context.getCommit(),
                    configBundle.getConfigPath(),
                    configBundle.getValidAYamlConfig()
            );
        }

        private Map<String, TriggerCfg> getActionsTriggeredOnPr(
                DiscoveryContext context,
                Map<String, ActionConfig> actions
        ) {
            var result = new LinkedHashMap<String, TriggerCfg>();
            for (var action : actions.values()) {
                EntryStream.of(action.getTriggers()).forKeyValue((i, trigger) -> {
                    if (trigger.getOn() != TriggerConfig.On.PR) {
                        log.info("Action {}, trigger {} (flow {}) skipped: not a PR trigger",
                                action.getId(), i, action.getFlow());
                        return; // ---
                    }
                    Preconditions.checkState(
                            context.getUpstreamBranch() != null, "upstreamBranch cannot be null in PR"
                    );
                    var upstreamBranchType = context.getUpstreamBranch().getType();
                    var acceptedUpstreamBranches = trigger.getTargetBranchesOrDefault();
                    if (!acceptedUpstreamBranches.contains(upstreamBranchType)) {
                        log.info("Action {}, trigger {} (flow {}) skipped: pr to branch '{}' which type is {}," +
                                        " but trigger accepts only {}",
                                action.getId(), i, action.getFlow(), context.getUpstreamBranch(), upstreamBranchType,
                                acceptedUpstreamBranches
                        );
                        return; // ---
                    }
                    if (!triggers.isMatchesAnyFilter(context, trigger.getFilters())) {
                        log.info("Action {}, trigger {} (flow {}) skipped: don't match any filter",
                                action.getId(), i, action.getFlow());
                        return; // ---
                    }

                    var prev = result.putIfAbsent(action.getId(), new TriggerCfg(action, trigger));
                    if (prev != null) {
                        prev.orRequired(trigger); // At least one required trigger makes flow required
                    }
                });
            }

            return result;
        }


        private void sendConfigValidationResultToArcanum() {
            var configEntity = configBundle.getConfigEntity();
            boolean validConfigRequired = isValidConfigRequired(configEntity.getConfigPath());

            String aYamlArcanumLink = null;
            if (!configEntity.getStatus().isValidCiConfig() && affectedAYaml.getChangeType() == ConfigChangeType.NONE) {
                aYamlArcanumLink = pullRequestService.generateUrlForArcPathAtTrunkHead(configEntity.getConfigPath());
            }
            pullRequestService.sendConfigValidationResult(diffSet.getPullRequestId(), diffSet.getDiffSetId(),
                    configEntity, validConfigRequired, aYamlArcanumLink, prDiscoveryContext);
        }

        private boolean isValidConfigRequired(Path configPath) {
            var author = diffSet.getAuthor();
            if (affectedAYaml.getChangeType() != ConfigChangeType.NONE) {
                return true;
            }
            return configurationService.findLastValidConfig(configPath, ArcBranch.trunk())
                    .flatMap(ConfigBundle::getOptionalAYamlConfig)
                    .map(yamlConfig -> abcService.isMember(author, yamlConfig.getService()))
                    .orElse(false);
        }

        private DiscoveryContext createContext() {
            ArcCommit commit = arcService.getCommit(revision);
            return DiscoveryContext.builder()
                    .revision(revision)
                    .previousRevision(upstreamRevision)
                    .commit(commit)
                    .upstreamBranch(diffSet.getVcsInfo().getUpstreamBranch())
                    .featureBranch(diffSet.getVcsInfo().getFeatureBranch())
                    .configChange(affectedAYaml.getChangeType())
                    .configBundle(configBundle)
                    .discoveryType(DiscoveryType.DIR)
                    .affectedPathsProvider(arcService)
                    .build();
        }

    }

    private static class TriggerCfg {
        private final ActionConfig action;
        private boolean required;

        private TriggerCfg(@Nonnull ActionConfig action, @Nonnull TriggerConfig trigger) {
            this.action = action;
            this.required = Boolean.TRUE.equals(trigger.getRequired());
        }

        void orRequired(@Nonnull TriggerConfig trigger) {
            this.required = this.required || Boolean.TRUE.equals(trigger.getRequired());
        }
    }
}
