package ru.yandex.ci.engine.launch;

import java.nio.file.Path;
import java.time.Clock;
import java.util.Collection;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Builder;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.CleanupConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.ReleaseTitleSource;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.FlowRunConfig;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchDisplacementChange;
import ru.yandex.ci.core.launch.LaunchFlowDescription;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.engine.event.LaunchEvent;
import ru.yandex.ci.engine.launch.version.LaunchVersionService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.engine.definition.stage.StageGroupHelper;
import ru.yandex.ci.util.Overrider;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class LaunchService {
    public static final String METRICS_GROUP = "launch_service";
    public static final String LAUNCH_NUMBER_NAMESPACE = "launch";

    private static final Logger log = LoggerFactory.getLogger(LaunchService.class);

    private final CiMainDb db;
    private final RevisionNumberService revisionNumberService;
    private final ConfigurationService configurationService;
    private final BazingaTaskManager bazingaTaskManager;
    private final BranchService branchService;
    private final CommitRangeService commitRangeService;
    private final LaunchVersionService launchVersionService;
    private final Clock clock;
    private final EventPublisher eventPublisher;
    private final FlowVarsService flowVarsService;
    private final Statistics statistics;
    private final ArcService arcService;

    public LaunchService(
            CiMainDb db,
            RevisionNumberService revisionNumberService,
            ConfigurationService configurationService,
            BazingaTaskManager bazingaTaskManager,
            BranchService branchService,
            CommitRangeService commitRangeService,
            LaunchVersionService launchVersionService,
            Clock clock,
            EventPublisher eventPublisher,
            FlowVarsService flowVarsService,
            MeterRegistry meterRegistry,
            ArcService arcService) {
        this.db = db;
        this.revisionNumberService = revisionNumberService;
        this.configurationService = configurationService;
        this.bazingaTaskManager = bazingaTaskManager;
        this.branchService = branchService;
        this.commitRangeService = commitRangeService;
        this.launchVersionService = launchVersionService;
        this.clock = clock;
        this.eventPublisher = eventPublisher;
        this.flowVarsService = flowVarsService;
        this.statistics = new Statistics(meterRegistry);
        this.arcService = arcService;
    }

    public ConfigBundle getConfig(
            CiProcessId processId,
            OrderedArcRevision configRevision
    ) {
        return configurationService.getConfig(processId.getPath(), configRevision);
    }

    public ConfigBundle getLastValidConfig(
            CiProcessId processId,
            ArcBranch branch
    ) {
        return configurationService.getLastValidConfig(processId.getPath(), branch);
    }

    public Launch startRelease(
            CiProcessId processId,
            CommitId commit,
            ArcBranch selectedBranch,
            String triggeredBy,
            @Nullable OrderedArcRevision configRevision,
            boolean cancelOthers,
            boolean preventDisplacement,
            @Nullable FlowReference flowReference,
            boolean allowReleaseWithoutCommits,
            @Nullable LaunchId rollbackUsingLaunch,
            @Nullable String launchReason,
            @Nullable Common.FlowVars flowVars
    ) {

        OrderedArcRevision revision = revisionNumberService.getOrderedArcRevision(selectedBranch, commit);

        ConfigBundle bundle = configRevision == null
                ? getLastValidConfig(processId, revision.getBranch())
                : getConfig(processId, configRevision);

        return this.db.currentOrTx(() -> {

            if (cancelOthers) {
                Preconditions.checkState(processId.getType().isRelease(),
                        "expected release process, got %s", processId);

                var releaseConfig = bundle.getValidAYamlConfig().getCi().getRelease(processId.getSubId());
                cancelAll(processId, releaseConfig, selectedBranch, triggeredBy, "Cancelled by another release");
            }

            var parameters = LaunchParameters.builder()
                    .processId(processId)
                    .flowReference(flowReference)
                    .launchType(Launch.Type.USER)
                    .bundle(bundle)
                    .triggeredBy(triggeredBy)
                    .revision(revision)
                    .selectedBranch(selectedBranch)
                    .allowLaunchWithoutCommits(allowReleaseWithoutCommits)
                    .rollbackUsingLaunch(rollbackUsingLaunch)
                    .launchReason(launchReason)
                    .flowVars(flowVarsService.parse(flowVars))
                    .preventDisplacement(preventDisplacement)
                    .build();
            return createAndStartLaunch(parameters);
        });
    }

    public Launch startPrFlow(
            CiProcessId processId,
            OrderedArcRevision revision,
            String triggeredBy,
            ConfigBundle bundle,
            LaunchPullRequestInfo launchPullRequestInfo,
            LaunchMode launchMode,
            boolean notifyPullRequest,
            @Nullable JsonObject flowVars
    ) {
        return createAndStartLaunch(LaunchParameters.builder()
                .processId(processId)
                .launchType(Launch.Type.USER)
                .bundle(bundle)
                .triggeredBy(triggeredBy)
                .revision(revision)
                .selectedBranch(revision.getBranch())
                .launchPullRequestInfo(launchPullRequestInfo)
                .notifyPullRequest(notifyPullRequest)
                .launchMode(launchMode)
                .flowVars(flowVars)
                .build()
        );
    }

    public void startDelayedLaunches(Path configPath, ArcRevision revision) {
        var delayedLaunchIds = db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, revision));
        log.info("Launching {} delayed launches for config {}. Config revision {}",
                delayedLaunchIds.size(), configPath, revision);
        for (var delayedLaunchId : delayedLaunchIds) {
            startDelayedOrPostponedLaunch(delayedLaunchId);
        }
    }

    public void startDelayedOrPostponedLaunch(Launch.Id launchId) {
        db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            startDelayedOrPostponedLaunchInTx(launch);
        });
    }

    public void startDelayedOrPostponedLaunchInTx(Launch launch) {
        var launchId = launch.getLaunchId();
        var status = launch.getStatus();
        if (status != LaunchState.Status.DELAYED && status != LaunchState.Status.POSTPONE) {
            log.warn("Unable to start delayed or postponed launch {} with status [{}]", launchId, status);
            return; // ---
        }

        var flowInfo = launch.getFlowInfo();
        if (status == LaunchState.Status.DELAYED || flowInfo.getRuntimeInfo().getYavTokenUid() == null) {
            log.info("Refreshing {} with status [{}] with token", launchId, status);

            var path = VirtualCiProcessId.of(launch.getProcessId()).getResolvedPath();
            var bundle = configurationService.getConfig(path, flowInfo.getConfigRevision());

            if (status == LaunchState.Status.DELAYED) {
                var launchMode = LaunchMode.ofAction(
                        bundle,
                        launch.getProcessId(),
                        launch.getVcsInfo().getRevision().getBranch().isPr()
                );
                if (launchMode == LaunchMode.POSTPONE) {
                    log.info("Changing launch {} status to {}", launchId, LaunchMode.POSTPONE);
                    db.launches().save(
                            launch.withStatus(LaunchState.Status.POSTPONE)
                    );
                    return;
                }
            }

            var tokenUuid = getTokenUiid(bundle.getConfigEntity());

            var runtimeInfo = flowInfo.getRuntimeInfo();
            var newFlowInfo = flowInfo.toBuilder()
                    .runtimeInfo(runtimeInfo.toBuilder()
                            .yavTokenUid(tokenUuid)
                            .build())
                    .build();

            log.info("Starting launch {}", launchId);
            scheduleLaunchStartTask(launchId);
            db.launches().save(
                    launch.toBuilder()
                            .flowInfo(newFlowInfo)
                            .status(LaunchState.Status.STARTING)
                            .build()
            );
        } else {
            // If POSTPONE launch has configured yavTokenId - this is a delegated security
            log.info("Starting launch {}", launchId);
            scheduleLaunchStartTask(launchId);
            db.launches().save(launch.withStatus(LaunchState.Status.STARTING));
        }
    }

    public void cancelDelayedOrPostponedLaunchInTx(Launch launch) {
        var launchId = launch.getLaunchId();
        var status = launch.getStatus();
        if (status != LaunchState.Status.DELAYED && status != LaunchState.Status.POSTPONE) {
            log.warn("Unable to cancel delayed or postponed launch {} with status [{}]", launchId, status);
            return; // ---
        }
        db.launches().save(launch.withStatus(LaunchState.Status.CANCELED));
    }

    public Launch createAndStartLaunch(LaunchParameters parameters) {
        return db.currentOrTx(() -> {
            var launchAndAction = createLaunch(parameters);
            var launch = launchAndAction.launch;

            savePostponeLaunch(launchAndAction.launch, launchAndAction.config);

            if (parameters.getLaunchMode() == LaunchMode.NORMAL) {
                scheduleLaunchStartTask(launch.getLaunchId());
            }

            eventPublisher.publish(new LaunchEvent(launch, clock));
            return db.launches().save(launch);
        });
    }

    private void savePostponeLaunch(Launch launch, @Nullable ActionConfig actionConfig) {
        if (launch.getStatus() != LaunchState.Status.POSTPONE) {
            return; // ---
        }

        if (actionConfig == null) {
            log.warn("Unexpected state. Launch {} has {} status but no action config",
                    launch.getId(), launch.getStatus());
            return; // ---
        }

        var commit = launch.getVcsInfo().getCommit();
        if (commit == null || !commit.isTrunk()) {
            log.warn("Unexpected state. Launch {} has {} status but it's not from trunk: {}",
                    launch.getId(), launch.getStatus(), commit);
            return; // ---
        }

        var virtualType = VirtualType.of(launch.getProcessId());
        if (virtualType != null || actionConfig.getBinarySearchConfig() != null) {
            db.postponeLaunches().saveIfNotPresent(launch);
        }
    }

    @SuppressWarnings("MethodLength") // Все это нужно переделать, но чуть попозже
    private LaunchAndConfig createLaunch(LaunchParameters parameters) {
        final CiProcessId processId = parameters.getProcessId();
        final ConfigBundle bundle = parameters.getBundle();
        final LaunchMode launchMode = parameters.getLaunchMode();
        final OrderedArcRevision revision = parameters.getRevision();
        final Launch.Type launchType = parameters.getLaunchType();
        final String triggeredBy = parameters.getTriggeredBy();
        final CiConfig ciConfig = bundle.getValidAYamlConfig().getCi();
        final FlowReference flowReference = FlowReference.defaultIfNull(ciConfig,
                processId, parameters.getFlowReference());

        log.info("Creating launch {} at {} in branch {}, ref {}",
                processId, revision, parameters.getSelectedBranch(), flowReference);
        if (processId.getType().isRelease()) {
            var branchConfig = bundle.getValidReleaseConfigOrThrow(processId).getBranches();
            var forbidTrunkReleases = branchConfig != null && branchConfig.isForbidTrunkReleases();
            if (forbidTrunkReleases && parameters.getSelectedBranch().isTrunk()) {
                throw new LaunchCanNotBeStartedException(
                        "Cannot start launch in branch trunk, releases in trunk is forbidden by configuration in a.yaml"
                );
            }
        }

        var canBeStartedResult = commitRangeService.canStartLaunchAt(
                processId,
                revision,
                parameters.getSelectedBranch(),
                parameters.isAllowLaunchWithoutCommits(),
                // Rollback flows can be launched on earlier commits
                flowReference.getFlowType() == Common.FlowType.FT_ROLLBACK
        );
        if (!canBeStartedResult.isAllowed()) {
            throw new LaunchCanNotBeStartedException(
                    "Cannot start launch at revision %s in branch %s: %s".formatted(
                            revision.getCommitId(), parameters.getSelectedBranch(), canBeStartedResult.getReason()
                    ));
        }

        if (launchMode != LaunchMode.NORMAL) {
            Preconditions.checkState(parameters.getBundle().getStatus().isValidCiConfig());
        } else if (!bundle.isReadyForLaunch()) {
            throw new ConfigHasSecurityProblemsException(bundle);
        }

        LaunchId launchId = generateIdInNewTransactionIfRequired(processId);

        // selected branch can be switched to auto created implicit branch
        ArcBranch selectedBranch = createImplicitBranchIfNeeded(parameters, processId, bundle, revision, triggeredBy);
        var version = getVersion(processId, bundle, revision, launchId, selectedBranch);
        var aYamlConfig = bundle.getValidAYamlConfig();

        var flowToRunRef = FlowRunConfig.lookup(processId, ciConfig, flowReference, new FlowToRunRefMapper());

        var flowVars = flowVarsService.prepareFlowVarsFromUi(
                parameters.getFlowVars(),
                flowToRunRef.getFlowRunConfig().getFlowVars(),
                flowToRunRef.getFlowRunConfig().getFlowVarsUi()
        );

        var cleanupConfig = flowToRunRef.getCleanupConfig();

        var rollbackUsingLaunch = parameters.getRollbackUsingLaunch();
        var flowInfoAndConfig = createFlowInfo(
                processId,
                bundle,
                launchMode,
                flowVars,
                cleanupConfig,
                rollbackUsingLaunch,
                parameters.getDelegatedSecurity(),
                flowReference,
                selectedBranch
        );
        var flowInfo = flowInfoAndConfig.launchFlowInfo;

        var vcsInfo = createVcsInfo(
                processId,
                revision,
                selectedBranch,
                parameters.getLaunchPullRequestInfo(),
                rollbackUsingLaunch
        );

        String titlePrefix = createTitlePrefix(processId, bundle, launchType, flowInfo);

        String title;
        var rollbackToVersion = flowInfo.getRollbackToVersion();
        if (rollbackToVersion != null) {
            title = "Rollback: %s #%s (#%s)".formatted(titlePrefix, version.asString(), rollbackToVersion.asString());
        } else {
            title = "%s #%s".formatted(titlePrefix, version.asString());
        }

        Set<Integer> cancelledReleases;
        Set<Integer> displacedReleases;
        if (processId.getType().isRelease()) {
            var result = commitRangeService.getCancelledAndDisplacedLaunches(
                    processId,
                    vcsInfo.getRevision(),
                    vcsInfo.getPreviousRevision());
            cancelledReleases = result.getCancelled();
            displacedReleases = result.getDisplaced();
        } else {
            cancelledReleases = Set.of();
            displacedReleases = Set.of();
        }
        var hasDisplacement = hasDisplacementOptions(processId, aYamlConfig.getCi());

        var status = switch (launchMode) {
            case NORMAL -> LaunchState.Status.STARTING;
            case DELAY -> LaunchState.Status.DELAYED;
            case POSTPONE -> LaunchState.Status.POSTPONE;
        };
        var now = clock.instant();
        var launchBuilder = Launch.builder()
                .launchId(launchId)
                .type(launchType)
                .notifyPullRequest(parameters.isNotifyPullRequest())
                .triggeredBy(triggeredBy)
                .created(now)
                .activityChanged(now)
                .vcsInfo(vcsInfo)
                .flowInfo(flowInfo)
                .title(title)
                .tags(getPredefinedTags(bundle, processId))
                .project(aYamlConfig.getService())
                .status(status)
                .statusText("")
                .cancelledReleases(cancelledReleases)
                .displacedReleases(displacedReleases)
                .version(version)
                .hasDisplacement(hasDisplacement)
                .launchReason(parameters.getLaunchReason());

        if (parameters.preventDisplacement) {
            var change = LaunchDisplacementChange.of(Common.DisplacementState.DS_DENY, triggeredBy, clock.instant());
            log.info("Change displacement state: {}", change);
            launchBuilder.displacementChange(change);
        }

        var launch = launchBuilder.build();

        if (processId.getType() == CiProcessId.Type.RELEASE) {
            if (db.launches().hasLaunchesByVersion(processId, version)) {
                throw new IllegalStateException("Launch with this version already exists in database: (%s, %s)"
                        .formatted(processId, version));
            }
        }

        branchService.updateTimelineBranchAndLaunchItems(launch);
        if (parameters.isSkipUpdatingDiscoveredCommit()) {
            log.info("Skip updating discsovered commit table");
        } else {
            db.discoveredCommit().updateOrCreate(processId, revision,
                    stateOptional -> stateOptional
                            .orElseThrow(() -> new IllegalStateException(
                                    "Discovered commit not found for (%s, %s)".formatted(processId, revision)
                            ))
                            .toBuilder()
                            .launchId(launchId)
                            .build()
            );
        }
        statistics.getReleasesStarted().increment();
        return new LaunchAndConfig(launch, flowInfoAndConfig.config);
    }

    private static Collection<String> getPredefinedTags(ConfigBundle bundle, CiProcessId processId) {
        if (processId.getType().isRelease()) {
            return bundle.getValidReleaseConfigOrThrow(processId).getTags();
        }
        var actionId = processId.getSubId();
        return bundle.getValidAYamlConfig().getCi().findAction(actionId)
                .map(ActionConfig::getTags)
                .orElse(List.of());
    }

    private ArcBranch createImplicitBranchIfNeeded(
            LaunchParameters parameters,
            CiProcessId processId,
            ConfigBundle bundle,
            OrderedArcRevision revision,
            String triggeredBy
    ) {
        ArcBranch selectedBranch;
        if (shouldCreateImplicitReleaseBranch(processId, bundle, parameters.getSelectedBranch())) {
            Branch branch = branchService.createBranch(processId, revision, triggeredBy);
            selectedBranch = branch.getArcBranch();
            log.info("Created branch {}", selectedBranch);
        } else {
            selectedBranch = parameters.getSelectedBranch();
            log.info("Use user selected branch {}", selectedBranch);
        }
        return selectedBranch;
    }

    private Version getVersion(
            CiProcessId processId,
            ConfigBundle bundle,
            OrderedArcRevision revision,
            LaunchId launchId,
            ArcBranch selectedBranch
    ) {
        return switch (processId.getType()) {
            case RELEASE -> {
                Integer startVersion = bundle.getValidReleaseConfigOrThrow(processId).getStartVersion();
                yield launchVersionService.nextLaunchVersion(processId, revision, selectedBranch, startVersion);
            }
            case FLOW -> launchVersionService.genericVersion(launchId);
            case SYSTEM -> throw new IllegalStateException("Unsupported type: " + processId.getType());
        };
    }

    private String createTitlePrefix(CiProcessId processId,
                                     ConfigBundle bundle,
                                     Launch.Type launchType,
                                     LaunchFlowInfo flowInfo) {
        var config = bundle.getValidAYamlConfig();

        var releaseTitleSource = processId.getType().isRelease()
                ? config.getCi().getReleaseTitleSource()
                : ReleaseTitleSource.FLOW;

        return switch (launchType) {
            case USER -> releaseTitleSource == ReleaseTitleSource.FLOW ?
                    config.getCi().getFlow(flowInfo.getFlowId().getId()).getTitle() :
                    bundle.getValidReleaseConfigOrThrow(processId).getTitle();
            default -> throw new IllegalStateException("Unsupported launchType: " + launchType);
        };
    }

    public boolean shouldCreateImplicitReleaseBranch(CiProcessId processId, ConfigBundle bundle,
                                                     ArcBranch selectedBranch) {
        if (!selectedBranch.isTrunk()) {
            return false;
        }

        if (processId.getType() != CiProcessId.Type.RELEASE) {
            return false;
        }
        var release = bundle.getValidReleaseConfigOrThrow(processId);

        return release.getBranches().isEnabled() && release.getBranches().isAutoCreate();
    }

    private boolean hasDisplacementOptions(CiProcessId processId, CiConfig ci) {
        if (processId.getType() == CiProcessId.Type.RELEASE) {
            var releaseConfig = ci.getRelease(processId.getSubId());
            return releaseConfig.hasDisplacement();
        }
        return false;
    }

    private void scheduleLaunchStartTask(LaunchId launchId) {
        bazingaTaskManager.schedule(new LaunchStartTask(launchId));
    }

    private void cancelAll(
            CiProcessId processId,
            ReleaseConfig releaseConfig,
            ArcBranch selectedBranch,
            String cancelledBy,
            String reason
    ) {

        var activeLaunches = db.currentOrReadOnly(() -> db.launches().getActiveLaunches(processId));
        List<LaunchId> ids;
        if (releaseConfig.isIndependentStages()) {
            var activeStageGroup = StageGroupHelper.createStageGroupId(processId, selectedBranch);
            ids = activeLaunches.stream()
                    .filter(l -> l.getFlowInfo().getStageGroupId().equals(activeStageGroup))
                    .map(Launch::getLaunchId)
                    .toList();
            log.info("cancelling {} launches in stage group {} ({} not cancelled): {}",
                    activeLaunches.size(),
                    activeStageGroup,
                    activeLaunches.size() - ids.size(),
                    ids
            );
        } else {
            ids = activeLaunches.stream()
                    .map(Launch::getLaunchId)
                    .toList();
            log.info("cancelling {} launches: {}", activeLaunches.size(), ids);
        }

        for (LaunchId launchId : ids) {
            log.info("cancelling {}", launchId);
            cancel(launchId, cancelledBy, reason);
        }
    }

    public Launch cancel(LaunchId launchId, String cancelledBy, String reason) {
        return db.currentOrTx(() -> {
            Launch launch = db.launches().get(launchId);

            bazingaTaskManager.schedule(new LaunchCancelTask(launchId));

            if (!launch.getStatus().isTerminal()) {

                Launch cancellingLaunch = launch.toBuilder()
                        .status(LaunchState.Status.CANCELLING)
                        .cancelledBy(cancelledBy)
                        .cancelledReason(reason)
                        .build();

                return db.launches().save(cancellingLaunch);
            }

            return launch;
        });
    }

    public Launch changeLaunchDisplacementState(
            LaunchId launchId, @Nonnull Common.DisplacementState state, @Nonnull String changedBy) {
        var change = LaunchDisplacementChange.of(state, changedBy, clock.instant());
        log.info("Change displacement state for {}: {}", launchId, change);
        return db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);

            if (launch.getStatus().isTerminal()) {
                log.info("Launch state is terminal, skip changing state: {}", launch.getStatus());
                return launch;
            }

            var updatedLaunch = launch.toBuilder().displacementChange(change).build();
            db.launches().save(updatedLaunch);
            return updatedLaunch;
        });
    }

    private LaunchVcsInfo createVcsInfo(
            CiProcessId processId,
            OrderedArcRevision revision,
            ArcBranch selectedBranch,
            @Nullable LaunchPullRequestInfo pullRequestInfo,
            @Nullable LaunchId rollbackUsingLaunch
    ) {
        // TOOD: https://st.yandex-team.ru/CI-2388
        var commit = db.arcCommit().find(revision.toCommitId())
                .orElseGet(() -> arcService.getCommit(revision));

        return switch (processId.getType()) {
            case RELEASE -> createReleaseVcsInfo(processId, commit, revision, selectedBranch, rollbackUsingLaunch);
            case FLOW -> createActionVcsInfo(revision, commit, pullRequestInfo, selectedBranch);
            case SYSTEM -> throw new IllegalStateException("Unsupported type: " + processId.getType());
        };
    }

    private LaunchVcsInfo createActionVcsInfo(
            OrderedArcRevision revision,
            @Nonnull ArcCommit arcCommit,
            @Nullable LaunchPullRequestInfo pullRequestInfo,
            ArcBranch selectedBranch) {

        OrderedArcRevision previousRevision = null;
        if (pullRequestInfo != null) {
            previousRevision = revisionNumberService.getOrderedArcRevision(
                    pullRequestInfo.getVcsInfo().getUpstreamBranch(),
                    pullRequestInfo.getVcsInfo().getUpstreamRevision()
            );
        } else if (!arcCommit.getParents().isEmpty()) {
            var parentCommit = arcService.getCommit(ArcRevision.of(arcCommit.getParents().get(0)));
            previousRevision = revisionNumberService.getOrderedArcRevision(selectedBranch, parentCommit);
        }
        return LaunchVcsInfo.builder()
                .revision(revision)
                .commit(arcCommit)
                .previousRevision(previousRevision)
                .pullRequestInfo(pullRequestInfo)
                .selectedBranch(selectedBranch)
                .build();

    }

    private LaunchVcsInfo createReleaseVcsInfo(
            CiProcessId processId,
            @Nonnull ArcCommit arcCommit,
            OrderedArcRevision revision,
            ArcBranch selectedBranch,
            @Nullable LaunchId rollbackUsingLaunch
    ) {
        CommitRangeService.Range range = rollbackUsingLaunch != null
                ? new CommitRangeService.Range(revision, revision, 0)
                : commitRangeService.getCommitsToCapture(processId, selectedBranch, revision);

        OrderedArcRevision stableRevision = db.launches().getLastFinishedLaunch(processId)
                .map(Launch::getVcsInfo)
                .map(LaunchVcsInfo::getRevision)
                .orElse(null);

        ReleaseVcsInfo releaseVscInfo = ReleaseVcsInfo.builder()
                .previousRevision(range.getPreviousRevision())
                .stableRevision(stableRevision)
                .build();

        return LaunchVcsInfo.builder()
                .revision(range.getRevision())
                .previousRevision(range.getPreviousRevision())
                .commitCount(range.getCount())
                .commit(arcCommit)
                .releaseVcsInfo(releaseVscInfo)
                .selectedBranch(selectedBranch)
                .build();
    }

    private LaunchFlowInfoAndConfig createFlowInfo(
            CiProcessId processId,
            ConfigBundle bundle,
            LaunchMode launchMode,
            @Nullable JsonObject flowVars,
            @Nullable CleanupConfig cleanupConfig,
            @Nullable LaunchId rollbackUsingLaunch,
            @Nullable DelegatedSecurity delegatedSecurity,
            FlowReference flowReference,
            ArcBranch branch
    ) {
        var aYamlConfig = bundle.getValidAYamlConfig();

        CiConfig ciConfig = aYamlConfig.getCi();
        var flowId = flowReference.getFlowId();

        String stageGroupId = StageGroupHelper.createStageGroupId(processId);

        RequirementsConfig requirementsConfig;
        RuntimeConfig runtimeConfig;
        ActionConfig action = null;
        switch (processId.getType()) {
            case FLOW -> {
                var actionOpt = ciConfig.findAction(processId.getSubId());
                if (actionOpt.isEmpty()) {
                    // запустили флоу, но при этом для него нет экшена
                    // такое бывает как минимум в тестах, ну что ж
                    runtimeConfig = ciConfig.getRuntime();
                    requirementsConfig = ciConfig.getRequirements();
                } else {
                    action = actionOpt.get();
                    runtimeConfig = Overrider.overrideNullable(
                            ciConfig.getRuntime(),
                            action.getRuntimeConfig()
                    );
                    requirementsConfig = Overrider.overrideNullable(
                            ciConfig.getRequirements(),
                            action.getRequirements()
                    );
                }
            }
            case RELEASE -> {
                var release = ciConfig.getRelease(processId.getSubId());
                if (release.isIndependentStages()) {
                    stageGroupId = StageGroupHelper.createStageGroupId(processId, branch);
                }
                runtimeConfig = Overrider.overrideNullable(ciConfig.getRuntime(), release.getRuntimeConfig());
                requirementsConfig = Overrider.overrideNullable(ciConfig.getRequirements(), release.getRequirements());
            }
            default -> {
                runtimeConfig = ciConfig.getRuntime();
                requirementsConfig = ciConfig.getRequirements();
            }
        }

        var configEntity = bundle.getConfigEntity();
        var runtimeInfo = createRuntimeInfo(
                configEntity,
                runtimeConfig,
                requirementsConfig,
                launchMode,
                delegatedSecurity
        );

        LaunchFlowDescription flowLaunchDescription;
        Version rollbackToVersion;
        var flow = ciConfig.findFlow(flowId);
        if (flow.isEmpty()) {
            flowLaunchDescription = null;
            rollbackToVersion = null;
        } else {
            var flowType = flowReference.getFlowType();
            if (flowType == Common.FlowType.FT_ROLLBACK) {
                if (rollbackUsingLaunch == null) {
                    throw new IllegalStateException("rollbackUsingLaunch parameter is required for flow type %s"
                            .formatted(flowType));
                }
            } else if (rollbackUsingLaunch != null) {
                throw new IllegalStateException("rollbackUsingLaunch parameter cannot be passed for flow type %s"
                        .formatted(flowType));
            }
            flowLaunchDescription = new LaunchFlowDescription(flow.get().getTitle(), flow.get().getDescription(),
                    flowType, rollbackUsingLaunch);

            if (rollbackUsingLaunch != null) {
                var rollbackToFlow = FlowLaunchServiceImpl.validateRollbackLaunch(db, rollbackUsingLaunch);
                rollbackToVersion = rollbackToFlow.getVersion();
            } else {
                rollbackToVersion = null;
            }
        }

        var flowVarsResource = flowVars != null ?
                JobResource.optional(JobResourceType.of(PropertiesSubstitutor.FLOW_VARS_KEY), flowVars) :
                null;

        var rollbackFlows = new LinkedHashSet<String>();
        if (processId.getType() == CiProcessId.Type.RELEASE && rollbackUsingLaunch == null) {
            var release = ciConfig.findRelease(processId.getSubId());
            if (release.isPresent()) {
                for (var rollbackFlow : release.get().getRollbackFlows()) {
                    if (rollbackFlow.acceptFlow(flowId)) {
                        rollbackFlows.add(rollbackFlow.getFlow());
                    }
                }
            }
        }

        var launchFlowInfo = LaunchFlowInfo.builder()
                .configRevision(configEntity.getRevision())
                .flowId(new FlowFullId(processId.getDir(), flowReference.getFlowId()))
                .stageGroupId(stageGroupId)
                .runtimeInfo(runtimeInfo)
                .flowDescription(flowLaunchDescription)
                .flowVars(flowVarsResource)
                .cleanupConfig(cleanupConfig)
                .rollbackFlows(List.copyOf(rollbackFlows))
                .rollbackToVersion(rollbackToVersion)
                .build();
        return new LaunchFlowInfoAndConfig(launchFlowInfo, action);
    }

    private static LaunchRuntimeInfo createRuntimeInfo(
            ConfigEntity securityConfigEntity,
            RuntimeConfig runtimeConfig,
            RequirementsConfig requirementsConfig,
            LaunchMode launchMode,
            @Nullable DelegatedSecurity delegatedSecurity
    ) {
        YavToken.Id tokenUuid;
        if (delegatedSecurity != null) {
            tokenUuid = delegatedSecurity.getToken();
        } else {
            if (launchMode != LaunchMode.NORMAL) {
                tokenUuid = null;
            } else {
                tokenUuid = getTokenUiid(securityConfigEntity);
            }
        }
        var sandboxOwner = delegatedSecurity != null
                ? delegatedSecurity.getSandboxOwner()
                : null; // default owner
        return LaunchRuntimeInfo.of(tokenUuid, sandboxOwner, runtimeConfig, requirementsConfig);
    }

    private static YavToken.Id getTokenUiid(ConfigEntity entity) {
        ConfigSecurityState securityState = entity.getSecurityState();
        Preconditions.checkState(
                securityState.getValidationStatus().isValid(),
                "Config %s (rev %s) invalid security state %s",
                entity.getConfigPath(), entity.getRevision(), securityState.getValidationStatus()
        );
        Preconditions.checkState(securityState.getYavTokenUuid() != null);
        return securityState.getYavTokenUuid();
    }

    private LaunchId generateIdInNewTransactionIfRequired(CiProcessId processId) {
        if (processId.getType() == CiProcessId.Type.FLOW) {
            /* Use short living new transaction instead of long living existing outer transaction to reduce
                number of OptimisticLockException.
               This type of processId doesn't require continuous ids. */
            return db.tx(() -> generateId(processId));
        }
        return generateId(processId);
    }

    private LaunchId generateId(CiProcessId processId) {
        int number = (int) db.counter().incrementAndGet(LAUNCH_NUMBER_NAMESPACE, processId.asString());
        return new LaunchId(processId, number);
    }

    @Value
    @NonNullApi
    private static class Statistics {

        Counter releasesStarted;

        Statistics(MeterRegistry meterRegistry) {
            this.releasesStarted = Counter.builder(LaunchService.METRICS_GROUP)
                    .tag("releases", "started")
                    .register(meterRegistry);
        }

    }

    public enum LaunchMode {
        NORMAL,
        DELAY, // Delayed but started automatically as soon as token is delegated
        POSTPONE; // Complete delay, no autostart until manual launch

        public static LaunchMode ofAction(ConfigBundle configBundle, CiProcessId processId, boolean isOnPrAction) {
            if (configBundle.getStatus() == ConfigStatus.SECURITY_PROBLEM) {
                return LaunchMode.DELAY;
            }

            // LaunchMode.POSTPONE is supported only for on-commit actions
            if (processId.getType() == CiProcessId.Type.FLOW && !isOnPrAction) {
                var action = configBundle.getValidAYamlConfig().getCi().getAction(processId.getSubId());
                if (action.hastMaxActiveCount()) {
                    return LaunchMode.POSTPONE;
                }
            }

            return LaunchMode.NORMAL;
        }

    }

    @Value
    @Builder
    public static class LaunchParameters {
        @Nonnull
        CiProcessId processId;

        @Nonnull
        Launch.Type launchType;

        @Nonnull
        ConfigBundle bundle;

        @Nonnull
        String triggeredBy;

        @Nonnull
        OrderedArcRevision revision;

        @Nonnull
        ArcBranch selectedBranch;

        @Nonnull
        LaunchMode launchMode;

        @Nullable
        LaunchPullRequestInfo launchPullRequestInfo;

        boolean notifyPullRequest;

        @Nullable
        FlowReference flowReference;

        boolean allowLaunchWithoutCommits;

        @Nullable
        LaunchId rollbackUsingLaunch;

        @Nullable
        String launchReason;

        @Nullable
        JsonObject flowVars;

        @Nullable
        DelegatedSecurity delegatedSecurity;

        boolean preventDisplacement;

        boolean skipUpdatingDiscoveredCommit;

        public static class Builder {
            {
                launchMode = LaunchMode.NORMAL;
                allowLaunchWithoutCommits = true;
            }
        }

    }

    @Value
    public static class DelegatedSecurity {
        @Nonnull
        String service;

        @Nonnull
        YavToken.Id token;

        @Nonnull
        String sandboxOwner;
    }


    /**
     * Указатель на используемый флоу + параметры указателя
     */
    @Value(staticConstructor = "of")
    private static class FlowToRunRef {
        @Nonnull
        FlowRunConfig flowRunConfig;

        @Nullable
        CleanupConfig cleanupConfig;
    }

    private static class FlowToRunRefMapper implements FlowRunConfig.Mapper<FlowToRunRef> {
        @Override
        public FlowToRunRef fromAction(FlowRunConfig runConfig, ActionConfig actionConfig) {
            return new FlowToRunRef(runConfig, actionConfig.getCleanupConfig());
        }

        @Override
        public FlowToRunRef fromFlow(FlowRunConfig runConfig, FlowConfig flowConfig) {
            return new FlowToRunRef(runConfig, null);
        }

        @Override
        public FlowToRunRef fromReleaseConfig(FlowRunConfig runConfig, ReleaseConfig releaseConfig) {
            return new FlowToRunRef(runConfig, null);
        }

        @Override
        public FlowToRunRef fromReleaseFlowReference(FlowRunConfig runConfig, FlowWithFlowVars flowWithFlowVars) {
            return new FlowToRunRef(runConfig, null);
        }

    }


    @Value
    private static class LaunchFlowInfoAndConfig {
        LaunchFlowInfo launchFlowInfo;
        @Nullable
        ActionConfig config;
    }

    @Value
    private static class LaunchAndConfig {
        Launch launch;
        @Nullable
        ActionConfig config;
    }
}
