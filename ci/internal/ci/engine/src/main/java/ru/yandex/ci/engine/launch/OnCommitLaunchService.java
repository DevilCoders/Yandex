package ru.yandex.ci.engine.launch;

import java.time.Clock;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import org.apache.commons.lang3.StringUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitNotFoundException;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.VirtualConfigState;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.pr.PullRequestDiffSetNotFoundException;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ActionConfigState.TestId;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.launch.LaunchService.LaunchParameters;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@RequiredArgsConstructor
public class OnCommitLaunchService {

    private static final Logger log = LoggerFactory.getLogger(OnCommitLaunchService.class);

    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final PullRequestService pullRequestService;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final RevisionNumberService revisionNumberService;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final Clock clock;
    @Nonnull
    private final Set<String> whiteListProjects;

    public Launch startFlow(StartFlowParameters startFlowParameters) {
        return new StartProcessor(startFlowParameters).startFlow();
    }

    public Optional<Launch> autoStartFlow(
            CiProcessId processId,
            OrderedArcRevision revision,
            ConfigBundle configBundle,
            LaunchMode launchMode
    ) {
        checkConfigStatus(configBundle, launchMode);

        DiscoveredCommit discoveredCommit = db.currentOrReadOnly(() ->
                db.discoveredCommit().findCommit(processId, revision)
                        .orElseThrow(() -> new IllegalStateException(
                                "DiscoveredCommit not found: %s %s".formatted(processId, revision)
                        ))
        );
        return discoveredCommit.getState()
                .getLaunchIds()
                .stream()
                .filter(it -> it.getProcessId().equals(processId))
                .findFirst()
                .map(existingLaunchId -> {
                    log.info("DiscoveredCommit {} already has launch of flow {}", discoveredCommit, processId);
                    return Optional.of(db.currentOrReadOnly(() -> db.launches().get(existingLaunchId)));
                })
                .orElseGet(() -> {
                    if (!isProjectInWhiteList(configBundle)) {
                        log.info("Starting flow {} on commit {} skipped, cause project not in white list {}",
                                processId, revision, whiteListProjects);
                        return Optional.empty();
                    }
                    log.info("Starting flow {} on commit {}", processId, revision);
                    Launch launch = launchService.createAndStartLaunch(LaunchParameters.builder()
                            .processId(processId)
                            .launchType(Launch.Type.USER)
                            .bundle(configBundle)
                            .triggeredBy(UserUtils.loginForInternalCiProcesses())
                            .revision(revision)
                            .selectedBranch(revision.getBranch())
                            .launchMode(launchMode)
                            .allowLaunchWithoutCommits(false)
                            .build()
                    );
                    log.info("Launch created {} for flow {} on commit {}", launch.getId(), processId, revision);
                    return Optional.of(launch);
                });
    }


    private boolean isProjectInWhiteList(ConfigBundle configBundle) {
        String project = configBundle.getValidAYamlConfig().getService();
        return whiteListProjects.isEmpty() || whiteListProjects.contains(project);
    }

    public Common.FlowDescription getFlowDescription(CiProcessId processId, OrderedArcRevision configRevision) {
        Preconditions.checkArgument(processId.getType() == CiProcessId.Type.FLOW,
                "unexpected process type %s", processId);

        ConfigBundle configBundle = configurationService.getConfig(processId.getPath(), configRevision);
        var id = processId.getSubId();
        var ciConfig = configBundle.getValidAYamlConfig().getCi();
        var actionOptional = ciConfig.findAction(id);
        var builder = Common.FlowDescription.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setFlowType(Common.FlowType.FT_DEFAULT);


        if (actionOptional.isPresent()) {
            var action = actionOptional.get();
            var flow = ciConfig.getFlow(action.getFlow());

            builder.setTitle(Objects.requireNonNullElse(action.getTitle(), flow.getTitle()));

            Stream.of(action.getDescription(), flow.getDescription())
                    .filter(Objects::nonNull)
                    .findFirst()
                    .ifPresent(builder::setDescription);

            Optional.ofNullable(action.getFlowVarsUi())
                    .map(ProtoMappers::toProtoFlowVarUi)
                    .ifPresent(builder::setFlowVarsUi);
        } else {
            var flow = ciConfig.getFlow(id);

            builder.setTitle(flow.getTitle());
            Optional.ofNullable(flow.getDescription())
                    .ifPresent(builder::setDescription);
        }
        return builder.build();
    }

    private void checkConfigStatus(ConfigBundle configBundle, LaunchMode launchMode) {
        Preconditions.checkArgument(
                (configBundle.getStatus() == ConfigStatus.READY) ||
                        (launchMode == LaunchMode.DELAY &&
                                (configBundle.getStatus() == ConfigStatus.SECURITY_PROBLEM)),
                "Config %s at revision %s has status %s and can't be used",
                configBundle.getConfigPath(), configBundle.getRevision(), configBundle.getStatus()
        );
    }

    @RequiredArgsConstructor
    private class StartProcessor {
        private final StartFlowParameters params;

        Launch startFlow() {
            if (!arcService.isCommitExists(params.revision)) {
                throw CommitNotFoundException.fromCommitId(params.revision);
            }

            var branch = params.branch;
            return switch (branch.getType()) {
                case PR -> launchFlowOnMergeRevisionOfPr();
                case TRUNK, RELEASE_BRANCH, USER_BRANCH, GROUP_BRANCH -> launchFlowOnTrunkOrReleaseBranch();
                default -> throw new IllegalArgumentException(
                        "type of branch %s is %s".formatted(branch.asString(), branch.getType())
                );
            };
        }

        private Launch launchFlowOnMergeRevisionOfPr() {
            var branch = params.branch;
            var diffSet = db.currentOrReadOnly(() ->
                            db.pullRequestDiffSetTable().findLatestByPullRequestId(branch.getPullRequestId()))
                    .orElseThrow(() -> new PullRequestDiffSetNotFoundException("Pull request not found " +
                            branch.getPullRequestId()));

            var configBundle = getConfig();

            var flowTitle = params.flowTitle;
            var processId = params.processId;
            var title = flowTitle != null
                    ? flowTitle
                    : pullRequestService.getPullRequestTitle(processId, configBundle.getValidAYamlConfig().getCi());

            LaunchPullRequestInfo launchPullRequestInfo =
                    pullRequestService.toLaunchPullRequestInfo(diffSet, processId, title);

            var revision = params.revision;
            OrderedArcRevision orderedRevision = revision.equals(diffSet.getOrderedMergeRevision().toRevision())
                    ? diffSet.getOrderedMergeRevision()
                    : revision.toOrdered(branch, -1, 0);

            updateDiscoveredCommit(orderedRevision);

            return startFlow(orderedRevision, configBundle, launchPullRequestInfo);
        }

        private Launch launchFlowOnTrunkOrReleaseBranch() {
            var branch = params.branch;
            Preconditions.checkState(!params.notifyPullRequest, "Cannot notify pull request for %s", branch.getType());
            var configBundle = getConfig();

            var revision = params.revision;
            OrderedArcRevision orderedRevision;
            if (branch.isTrunk() || branch.isRelease()) {
                orderedRevision = revisionNumberService.getOrderedArcRevision(branch, revision);
            } else {
                orderedRevision = revision.toOrdered(branch, -1, 0);
            }

            updateDiscoveredCommit(orderedRevision);

            return startFlow(orderedRevision, configBundle, null);
        }

        private void updateDiscoveredCommit(OrderedArcRevision orderedRevision) {
            db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                    params.processId,
                    orderedRevision,
                    optionalState -> optionalState.orElseGet(
                            () -> DiscoveredCommitState.builder()
                                    .manualDiscovery(true)
                                    .build()
                    )
            ));
        }

        private ConfigBundle getConfig() {
            var processId = params.processId;
            var virtualProcessId = VirtualCiProcessId.of(processId);
            var path = virtualProcessId.getResolvedPath();

            log.info("Resolved configuration to load: {} -> {}", processId, path);

            var configOrderedRevision = params.configOrderedRevision;
            ConfigBundle configBundle = configOrderedRevision == null
                    ? configurationService.getLastValidConfig(path, ArcBranch.trunk())
                    : configurationService.getConfig(path, configOrderedRevision);

            checkConfigStatus(configBundle, params.launchMode);

            return updateVirtualConfigState(configBundle, virtualProcessId);
        }

        @Nullable
        private TestId toTestId() {
            var flowVars = params.getFlowVars();
            if (flowVars == null) {
                return null;
            }

            var testInfo = flowVars.getAsJsonObject("testInfo");
            if (testInfo == null) {
                return null;
            }

            var suiteId = testInfo.getAsJsonPrimitive("suiteId").getAsString();
            var toolchain = testInfo.getAsJsonPrimitive("toolchain").getAsString();

            return TestId.of(suiteId, toolchain);
        }

        private ConfigBundle updateVirtualConfigState(
                ConfigBundle configBundle,
                VirtualCiProcessId virtualCiProcessId
        ) {
            var virtualType = virtualCiProcessId.getVirtualType();
            if (virtualType == null) {
                return configBundle; // --- Nothing to update
            }

            var ciProcessId = virtualCiProcessId.getCiProcessId();

            var path = ciProcessId.getPath();
            var delegatedSecurity = params.delegatedSecurity;
            var project = delegatedSecurity != null
                    ? delegatedSecurity.getService()
                    : virtualType.getService();

            var flowState = ActionConfigState.builder()
                    .flowId(ciProcessId.getSubId())
                    .title(ciProcessId.getSubId())
                    .showInActions(Boolean.TRUE)
                    .testId(toTestId())
                    .build();

            db.currentOrTx(() -> {
                var stateOptional = db.virtualConfigStates().find(path);
                VirtualConfigState newState;
                if (stateOptional.isEmpty()) {
                    var title = virtualType.getTitlePrefix() + " " +
                            StringUtils.removeStart(ciProcessId.getDir(), virtualType.getPrefix());
                    newState = VirtualConfigState.builder()
                            .id(VirtualConfigState.Id.of(path))
                            .virtualType(virtualType)
                            .project(project)
                            .title(title)
                            .action(flowState)
                            .created(clock.instant())
                            .updated(clock.instant())
                            .status(ConfigState.Status.OK)
                            .build();
                    log.info("Creating new VirtualConfigState for {}, project {}, flow {}",
                            newState.getId(), newState.getProject(), flowState.getFlowId());
                } else {
                    var state = stateOptional.get();
                    var stateBuilder = state.toBuilder()
                            .project(project)
                            .updated(clock.instant())
                            .status(ConfigState.Status.OK);

                    // Update action (Lombok builder API is not great)
                    var actions = state.getActions().stream()
                            .map(oldFlowState -> {
                                if (Objects.equals(flowState.getFlowId(), oldFlowState.getFlowId())) {
                                    return flowState;
                                } else {
                                    return oldFlowState;
                                }
                            })
                            .toList();
                    stateBuilder.clearActions();
                    stateBuilder.actions(actions);
                    newState = stateBuilder.build();
                    log.info("Updating VirtualConfigState for {}, project {} -> {}, flow {}",
                            newState.getId(), state.getProject(), newState.getProject(), flowState.getFlowId());
                }
                db.virtualConfigStates().save(newState);
            });

            return configBundle.withVirtualProcessId(virtualCiProcessId, project);
        }

        private Launch startFlow(
                OrderedArcRevision revision,
                ConfigBundle configBundle,
                @Nullable LaunchPullRequestInfo launchPullRequestInfo
        ) {

            if (!isProjectInWhiteList(configBundle)) {
                throw new ProjectNotInWhiteListException();
            }
            var processId = params.processId;
            var delegatedSecurity = params.delegatedSecurity;
            log.info("Starting flow {} on commit {}", processId, revision);
            if (delegatedSecurity != null) {
                log.info("Using delegated security: {}", delegatedSecurity);
            }
            Launch launch = launchService.createAndStartLaunch(LaunchParameters.builder()
                    .processId(processId)
                    .launchType(Launch.Type.USER)
                    .bundle(configBundle)
                    .triggeredBy(params.triggeredBy)
                    .notifyPullRequest(params.notifyPullRequest)
                    .launchPullRequestInfo(launchPullRequestInfo)
                    .revision(revision)
                    .selectedBranch(params.branch)
                    .flowVars(params.flowVars)
                    .delegatedSecurity(delegatedSecurity)
                    .skipUpdatingDiscoveredCommit(params.skipUpdatingDiscoveredCommit)
                    .launchMode(params.launchMode)
                    .build()
            );
            log.info("Launch created {} for flow {} on commit {}", launch.getId(), processId, revision);
            return launch;
        }
    }

    @Value
    @Builder
    public static class StartFlowParameters {
        @Nonnull
        CiProcessId processId;
        @Nonnull
        ArcBranch branch;
        @Nonnull
        ArcRevision revision;
        @Nullable
        OrderedArcRevision configOrderedRevision;
        @Nonnull
        String triggeredBy;
        @Nonnull
        LaunchMode launchMode;
        boolean notifyPullRequest;
        @Nullable
        JsonObject flowVars;
        @Nullable
        String flowTitle;
        @Nullable
        LaunchService.DelegatedSecurity delegatedSecurity;
        boolean skipUpdatingDiscoveredCommit;

        public static class Builder {
            {
                launchMode = LaunchMode.NORMAL;
            }
        }
    }

}
