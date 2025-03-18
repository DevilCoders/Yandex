package ru.yandex.ci.api.controllers.frontend;

import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Iterables;
import com.google.protobuf.TextFormat;
import io.grpc.Status;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import org.apache.commons.lang3.tuple.Pair;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.GetCommitsRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.GetCommitsResponse;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.proto.Common.FlowType;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.ReleaseBranchId;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.timeline.ReleaseCommit;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.launch.FlowLaunchServiceImpl;
import ru.yandex.ci.engine.launch.LaunchCanNotBeStartedException;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.CommitFetchService.CommitOffset;
import ru.yandex.ci.engine.timeline.CommitFetchService.CommitRange;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.util.OffsetResults;

@RequiredArgsConstructor
public class ReleaseController extends ReleaseServiceGrpc.ReleaseServiceImplBase {

    private static final Logger log = LoggerFactory.getLogger(ReleaseController.class);

    private final LaunchService launchService;
    private final PermissionsService permissionsService;
    private final CiDb db;
    private final FlowLaunchApiService flowLaunchApiService;
    private final AbcService abcService;
    private final AutoReleaseService autoReleaseService;
    private final BranchService branchService;
    private final TimelineService timelineService;
    private final CommitRangeService commitRangeService;
    private final CommitFetchService commitFetchService;

    @Override
    public void getReleaseProcessStateVersion(
            FrontendReleaseApi.GetReleaseProcessStateVersionRequest request,
            StreamObserver<FrontendReleaseApi.GetReleaseProcessStateVersionResponse> responseObserver
    ) {
        //Заглушка, которая гарантирует, что фронт будет обновлятся
        try {
            TimeUnit.SECONDS.sleep(45);
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        }
        boolean updated = true;
        long stateVersion = Math.max(1, request.getKnownStateVersion()) + 1;

        responseObserver.onNext(
                FrontendReleaseApi.GetReleaseProcessStateVersionResponse.newBuilder()
                        .setReleaseProcessId(request.getReleaseProcessId())
                        .setStateVersion(stateVersion)
                        .setUpdated(updated)
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void getReleases(
            FrontendReleaseApi.GetReleasesRequest request,
            StreamObserver<FrontendReleaseApi.GetReleasesResponse> responseObserver
    ) {
        OffsetResults<Launch> launches = OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        request.getLimit(),
                        limit -> db.launches().getLaunches(
                                ProtoMappers.toCiProcessId(request.getReleaseProcessId()),
                                request.getOffsetNumber(),
                                limit,
                                request.getDontReturnCancelled()
                        )
                )
                .fetch();

        var launchMap = TimelineController.enrichLaunches(db, launches.items());

        responseObserver.onNext(
                FrontendReleaseApi.GetReleasesResponse.newBuilder()
                        .addAllReleases(
                                launches.items().stream()
                                        .map(item ->
                                                ProtoMappers.toProtoReleaseLaunch(item, launchMap.get(item.getId()),
                                                        false))
                                        .collect(Collectors.toList())
                        )
                        .setOffset(ProtoMappers.toProtoOffset(launches))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void getRelease(
            FrontendReleaseApi.GetReleaseRequest request,
            StreamObserver<FrontendReleaseApi.GetReleaseResponse> responseObserver
    ) {

        LaunchId launchId = ProtoMappers.toLaunchId(request.getId());
        CiProcessId processId = launchId.getProcessId();

        var versionOp = ProtoMappers.fromProtoVersion(request.getVersion());
        Launch launch;
        if (launchId.getNumber() > 0 || versionOp.isEmpty()) {
            launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        } else {
            launch = db.currentOrReadOnly(() -> db.launches().findLaunchesByVersion(processId, versionOp.get()))
                    .orElseThrow(() ->
                            GrpcUtils.notFoundException("Launch not found " + launchId +
                                    ", using version: " + TextFormat.shortDebugString(request.getVersion())));
        }

        Common.ReleaseLaunch releaseLaunch = this.toProtoReleaseLaunch(launch);

        FrontendReleaseApi.GetReleaseResponse.Builder builder = FrontendReleaseApi.GetReleaseResponse.newBuilder()
                .setRelease(releaseLaunch);

        if (releaseLaunch.hasFlowLaunchId()) {
            builder.setLaunchState(flowLaunchApiService.getLaunchState(releaseLaunch.getFlowLaunchId()));
        }


        ConfigState configState = db.currentOrReadOnly(() -> db.configStates().get(processId.getPath()));

        Common.ReleaseState releaseState = getReleaseState(configState, launchId.getProcessId());
        builder.setReleaseState(releaseState);

        Common.Project project = getProject(configState.getProject());
        builder.setProject(project);

        responseObserver.onNext(builder.build());
        responseObserver.onCompleted();
    }

    private Common.ReleaseState getReleaseState(ConfigState configState, CiProcessId processId) {
        var releaseState = configState.getReleases().stream()
                .filter(state -> state.getReleaseId().equals(processId.getSubId()))
                .findFirst()
                .orElseThrow(() ->
                        GrpcUtils.notFoundException("Not found release process " + processId.asString())
                );

        var autoReleaseState = autoReleaseService.findAutoReleaseStateOrDefault(processId);
        var branches = releaseState.isReleaseBranchesEnabled()
                ? OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(ProjectController.TOP_N_BRANCHES_IN_PROJECT,
                        limit -> branchService.getTopBranches(processId, limit))
                .fetch()
                : null;

        return ProtoMappers.toProtoReleaseState(processId, releaseState, autoReleaseState, branches);
    }

    private Common.Project getProject(String project) {
        return abcService.getService(project)
                .map(ProtoMappers::toProtoProject)
                .orElseThrow(
                        () -> Status.NOT_FOUND.withDescription("No abc service with id: " + project)
                                .asRuntimeException()
                );
    }

    @Override
    public void getCommits(GetCommitsRequest request, StreamObserver<GetCommitsResponse> responseObserver) {
        CommitOffset offset = request.hasOffset() ?
                CommitOffset.of(
                        ArcBranch.ofBranchName(request.getOffset().getBranch()),
                        request.getOffset().getNumber()
                )
                : CommitOffset.of(ArcBranch.trunk(), request.getOffsetCommitNumber());

        // fixed in 8.31.6 https://github.com/checkstyle/checkstyle/issues/8779
        CommitRange range = switch (request.getIdCase()) {
            case RELEASE_LAUNCH_ID -> commitFetchService.buildCommitRangeForReleaseLaunch(
                    ProtoMappers.toLaunchId(request.getReleaseLaunchId()), false
            );
            case FREE_COMMITS, RELEASE_PROCESS_ID -> buildCommitRangeForFreeCommits(request);
            case BRANCH_COMMITS -> buildCommitRangeForBranchCommits(request.getBranchCommits());
            case ID_NOT_SET -> throw GrpcUtils.invalidArgumentException("Non of item id was set");
        };
        OffsetResults<ReleaseCommit> offsetResults = commitFetchService.fetchCommits(range, offset, request.getLimit());

        GetCommitsResponse.Builder responseBuilder = GetCommitsResponse.newBuilder()
                .setOffset(ProtoMappers.toProtoOffset(offsetResults));

        nextOffsetFor(offsetResults).ifPresent(responseBuilder::setNext);

        offsetResults.items().stream()
                .map(ProtoMappers::toProtoReleaseCommit)
                .forEach(responseBuilder::addReleaseCommits);

        if (allowStartReleaseWithoutCommits(request) && !offsetResults.hasMore()) {
            findDiscoveredCommitFromLastRelease(range)
                    .map(ProtoMappers::toProtoReleaseCommit)
                    .ifPresent(responseBuilder::setRestartableReleaseCommit);
        }

        responseObserver.onNext(responseBuilder.build());
        responseObserver.onCompleted();
    }

    private Optional<ReleaseCommit> findDiscoveredCommitFromLastRelease(CommitRange range) {
        return db.currentOrReadOnly(() ->
                commitRangeService.getLastReleaseCommitCanStartAt(range.getProcessId(), range.getFromBranch())
                        .flatMap(revision ->
                                db.discoveredCommit().findCommit(
                                        range.getProcessId(), revision
                                )
                        )
                        .flatMap(discoveredCommit ->
                                commitFetchService.enrichCommits(range.getProcessId(), List.of(discoveredCommit))
                                        .stream()
                                        .findFirst()
                        )
        );
    }

    private static Optional<FrontendReleaseApi.CommitOffset> nextOffsetFor(OffsetResults<ReleaseCommit> results) {
        if (results.hasMore()) {
            OrderedArcRevision lastRevision = Iterables.getLast(results.items()).getRevision();
            return Optional.of(FrontendReleaseApi.CommitOffset.newBuilder()
                    .setBranch(lastRevision.getBranch().asString())
                    .setNumber(lastRevision.getNumber())
                    .build());
        }
        return Optional.empty();
    }

    @Override
    public void getCommitsToCherryPick(FrontendReleaseApi.GetCommitsToCherryPickRequest request,
                                       StreamObserver<GetCommitsResponse> responseObserver) {
        var sourceBranch = ArcBranch.ofBranchName(request.getSourceBranch());
        var targetBranch = ArcBranch.ofBranchName(request.getTargetBranch().getBranch());
        var processId = ProtoMappers.toCiProcessId(request.getTargetBranch().getReleaseProcessId());

        CommitRange.Builder rangeBuilder = CommitRange.builder()
                .processId(processId)
                .fromBranch(sourceBranch);

        if (sourceBranch.equals(targetBranch)) {
            throw GrpcUtils.failedPreconditionException(
                    "source and target branch equals: '%s'".formatted(sourceBranch));
        }

        var sideBranch = sourceBranch.isTrunk() ? targetBranch : sourceBranch;
        var branchInfo = db.readOnly(() -> branchService.getBranch(sideBranch));
        if (branchInfo.isEmpty()) {
            throw GrpcUtils.notFoundException(
                    "branch '%s' not found".formatted(targetBranch));
        }
        var baseRevision = branchInfo.orElseThrow().getBaseRevision();

        rangeBuilder.toRevision(baseRevision);

        var offset = request.hasOffset() ?
                CommitOffset.of(
                        ArcBranch.ofBranchName(request.getOffset().getBranch()), request.getOffset().getNumber()
                )
                : CommitOffset.of(sourceBranch, -1);

        var responseBuilder = GetCommitsResponse.newBuilder();

        var commits = commitFetchService.fetchCommits(rangeBuilder.build(), offset, request.getLimit());

        commits.items().stream()
                .map(ProtoMappers::toProtoReleaseCommit)
                .forEach(responseBuilder::addReleaseCommits);

        responseBuilder.setOffset(ProtoMappers.toProtoOffset(commits));
        nextOffsetFor(commits).ifPresent(responseBuilder::setNext);

        responseObserver.onNext(responseBuilder.build());
        responseObserver.onCompleted();
    }

    private CommitRange buildCommitRangeForFreeCommits(GetCommitsRequest request) {
        ReleaseBranchId branchId = switch (request.getIdCase()) {
            case RELEASE_PROCESS_ID -> ReleaseBranchId.of(
                    ProtoMappers.toCiProcessId(request.getReleaseProcessId()),
                    ArcBranch.trunk()
            );
            case FREE_COMMITS -> ProtoMappers.toReleaseBranchId(request.getFreeCommits());
            default -> throw new IllegalArgumentException(request.getIdCase().toString());
        };
        log.info("Requesting free commits for {}", branchId);

        CommitRange.Builder builder = CommitRange.builder()
                .processId(branchId.getProcessId())
                .fromBranch(branchId.getBranch());

        db.currentOrReadOnly(() -> timelineService.getLastIn(branchId.getProcessId(), branchId.getBranch())
                        .map(TimelineItem::getArcRevision)
                        .map(rev -> Pair.of(rev.getBranch(), rev.getNumber()))
                        .or(() -> {
                            log.info("Not found any timeline items in " + branchId);
                            if (branchId.getBranch().isTrunk()) {
                                log.info("Branch is trunk, there are no more places to get timeline");
                                return Optional.empty();
                            }

                            Branch branch = branchService.getBranch(branchId.getBranch(), branchId.getProcessId());

                            log.info("Use range up to previousCommit in branch: {}",
                                    branch.getVcsInfo().getPreviousRevision());

                            return Optional.ofNullable(branch.getVcsInfo().getPreviousRevision())
                                    .map(revision -> Pair.of(revision.getBranch(), revision.getNumber()));
                        }))
                .ifPresent(branchAndNumber ->
                        builder.toBranch(branchAndNumber.getLeft())
                                .toCommitNumber(branchAndNumber.getRight())
                );

        return builder.build();
    }

    private CommitRange buildCommitRangeForBranchCommits(Common.BranchId branchCommits) {
        ReleaseBranchId branchId = ProtoMappers.toReleaseBranchId(branchCommits);
        log.info("Requesting commits for branch {}", branchId);

        Branch branch = db.currentOrReadOnly(() ->
                branchService.getBranch(branchId.getBranch(), branchId.getProcessId()));

        return CommitRange.builder()
                .processId(branchId.getProcessId())
                .fromRevision(branch.getInfo().getBaseRevision())
                .toRevision(branch.getVcsInfo().getPreviousRevision())
                .build();
    }

    private static boolean allowStartReleaseWithoutCommits(GetCommitsRequest request) {
        return request.getIdCase() == GetCommitsRequest.IdCase.FREE_COMMITS;
    }

    @Override
    public void getReleaseFlows(FrontendReleaseApi.GetReleaseFlowsRequest request,
                                StreamObserver<FrontendReleaseApi.GetReleaseFlowsResponse> responseObserver) {
        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        permissionsService.checkAccess(login, processId, PermissionScope.START_FLOW);

        var config = launchService.getConfig(processId, ProtoMappers.toOrderedArcRevision(request.getConfigRevision()));

        var response = FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder();

        // Список flow пока имеет смысл формировать только при запуске релиза
        if (processId.getType() == CiProcessId.Type.RELEASE) {
            var release = config.getValidReleaseConfigOrThrow(processId);

            boolean hasDisplacement = release.hasDisplacement();
            boolean displacementEnabled = switch (release.getDisplacementOnStart()) {
                case AUTO -> hasDisplacement && release.getAuto() != null && release.getAuto().isEnabled();
                case ENABLED -> hasDisplacement;
                case DISABLED -> false;
            };

            response.setHasDisplacement(hasDisplacement);
            response.setDisplacementEnabled(displacementEnabled);

            var ciConfig = config.getValidAYamlConfig().getCi();

            FlowCollector addFlow = (flowId, flowType, flowVarsUi) -> {
                var flow = ciConfig.getFlow(flowId);
                var newFlow = response.addFlowsBuilder()
                        // TODO: переделать, тут должен быть просто flowId
                        .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId, flowId))
                        .setFlowType(flowType)
                        .setTitle(flow.getTitle());
                if (flow.getDescription() != null) {
                    newFlow.setDescription(flow.getDescription());
                }

                if (flowVarsUi != null) {
                    newFlow.setFlowVarsUi(ProtoMappers.toProtoFlowVarUi(flowVarsUi));
                }
            };
            var rollbackLaunchId = getLaunch(request);
            if (rollbackLaunchId != null) {
                // Отфильтруем список rollback flow
                // Пока нет вариантов кроме как загрузить launch и посмотреть на него
                var rollbackLaunch = FlowLaunchServiceImpl.validateRollbackLaunch(db, rollbackLaunchId);
                var rollbackFlowId = rollbackLaunch.getFlowInfo().getFlowId().getId();

                release.getRollbackFlows().stream()
                        .filter(ref -> ref.acceptFlow(rollbackFlowId))
                        .forEach(ref -> addFlow.accept(ref.getFlow(), FlowType.FT_ROLLBACK, ref.getFlowVarsUi()));
            } else {
                addFlow.accept(release.getFlow(), FlowType.FT_DEFAULT, release.getFlowVarsUi());
                release.getHotfixFlows().forEach(ref ->
                        addFlow.accept(ref.getFlow(), FlowType.FT_HOTFIX, ref.getFlowVarsUi())
                );
            }
        }

        responseObserver.onNext(response.build());
        responseObserver.onCompleted();
    }

    @FunctionalInterface
    private interface FlowCollector {
        void accept(String flowId, FlowType type, FlowVarsUi flowVarsUi);
    }

    @Override
    public void startRelease(
            FrontendReleaseApi.StartReleaseRequest request,
            StreamObserver<FrontendReleaseApi.StartReleaseResponse> responseObserver
    ) {
        var startConfig = fillStartConfig(request);

        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());

        // TODO: check if we can use custom flow (starting releases in branches)
        var flowReference = request.hasFlowProcessId()
                ? FlowReference.from(request.getFlowProcessId(), request.getFlowType())
                : null;

        if (request.hasFlowProcessId() && !request.getFlowProcessId().getDir().equals(processId.getDir())) {
            throw GrpcUtils.invalidArgumentException(
                    "flow_process_id %s invalid, path mismatched with process %s".formatted(
                            request.getFlowProcessId(), processId.getDir()
                    ));
        }

        permissionsService.checkAccess(login, processId, getStartReleaseScope(request.getFlowType()));

        Launch launch;
        try {
            launch = launchService.startRelease(
                    processId,
                    startConfig.commit,
                    startConfig.selectedBranch,
                    login,
                    startConfig.configRevision,
                    request.getCancelOthers(),
                    request.getPreventDisplacement(),
                    flowReference,
                    true,
                    startConfig.rollbackUsingLaunch,
                    startConfig.launchReason,
                    request.hasFlowVars() ? request.getFlowVars() : null
            );
        } catch (LaunchCanNotBeStartedException e) {
            throw GrpcUtils.failedPreconditionException(e);
        }
        responseObserver.onNext(
                FrontendReleaseApi.StartReleaseResponse.newBuilder()
                        .setReleaseLaunch(this.toProtoReleaseLaunch(launch))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void startRollbackRelease(FrontendReleaseApi.StartRollbackReleaseRequest request,
                                     StreamObserver<FrontendReleaseApi.StartReleaseResponse> responseObserver) {
        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getRollbackLaunch().getReleaseProcessId());
        permissionsService.checkAccess(login, processId, PermissionScope.ROLLBACK_FLOW);

        var launchId = getLaunch(request.getRollbackLaunch());
        Preconditions.checkState(launchId != null, "RollbackLaunch is mandatory");

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));

        var releaseProcessId = ProtoMappers.toProtoReleaseProcessId(launch.getProcessId());
        var configRevision = ProtoMappers.hasOrderedArcRevision(request.getConfigRevision())
                ? request.getConfigRevision()
                : ProtoMappers.toProtoOrderedArcRevision(launch.getFlowInfo().getConfigRevision());

        var vcsInfo = launch.getVcsInfo();
        var revision = vcsInfo.getRevision();
        var branch = Objects.requireNonNullElse(vcsInfo.getSelectedBranch(), revision.getBranch());
        var startReleaseRequest = FrontendReleaseApi.StartReleaseRequest.newBuilder()
                .setReleaseProcessId(releaseProcessId)
                .setConfigRevision(configRevision)
                .setBranch(branch.asString())
                .setCommit(ProtoMappers.toCommitId(revision))
                .setCancelOthers(request.getCancelOthers())
                .setFlowProcessId(request.getFlowProcessId())
                .setFlowType(FlowType.FT_ROLLBACK)
                .setRollbackLaunch(request.getRollbackLaunch())
                .setLaunchReason(request.getRollbackReason())
                .build();

        log.info("Prepared startReleaseRequest: {}", TextFormat.shortDebugString(startReleaseRequest));

        if (request.getDisableAutorelease()) {
            log.info("Disable auto releases if possible");
            autoReleaseService.updateAutoReleaseState(launch.getProcessId(), false, login,
                    request.getRollbackReason());
        }
        this.startRelease(startReleaseRequest, responseObserver);

    }

    @Override
    public void cancelRelease(
            FrontendReleaseApi.CancelReleaseRequest request,
            StreamObserver<FrontendReleaseApi.CancelReleaseResponse> responseObserver
    ) {
        var launchId = checkAccess(request.getId(), PermissionScope.CANCEL_FLOW);
        Launch launch = launchService.cancel(launchId, AuthUtils.getUsername(), request.getReason());

        responseObserver.onNext(
                FrontendReleaseApi.CancelReleaseResponse.newBuilder()
                        .setRelease(this.toProtoReleaseLaunch(launch))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void updateAutoReleaseSettings(
            FrontendReleaseApi.UpdateAutoReleaseSettingsRequest request,
            StreamObserver<FrontendReleaseApi.UpdateAutoReleaseSettingsResponse> responseObserver
    ) {
        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        permissionsService.checkAccess(login, processId, PermissionScope.TOGGLE_AUTORUN);

        var autoReleaseState = autoReleaseService.updateAutoReleaseState(
                processId, request.getEnabled(), login, request.getMessage());
        if (autoReleaseState.isEmpty()) {
            // TODO: add details like config revision, is config valid or not, etc
            throw GrpcUtils.permissionDeniedException("Auto release is disabled in configs, cannot change state");
        }

        responseObserver.onNext(
                FrontendReleaseApi.UpdateAutoReleaseSettingsResponse.newBuilder()
                        .setAuto(ProtoMappers.toProtoAutoReleaseState(autoReleaseState.get()))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void changeDisplacementState(
            FrontendReleaseApi.DisplacementChangeRequest request,
            StreamObserver<FrontendReleaseApi.DisplacementChangeResponse> responseObserver) {
        var launchId = checkAccess(request.getId(), PermissionScope.MODIFY);
        var login = AuthUtils.getUsername();
        Launch launch = launchService.changeLaunchDisplacementState(launchId, request.getState(), login);

        responseObserver.onNext(
                FrontendReleaseApi.DisplacementChangeResponse.newBuilder()
                        .setRelease(this.toProtoReleaseLaunch(launch))
                        .build()
        );
        responseObserver.onCompleted();

    }

    //

    private Common.ReleaseLaunch toProtoReleaseLaunch(Launch launch) {
        return db.currentOrReadOnly(() -> {
            var flowLaunchId = launch.getFlowLaunchId();
            if (flowLaunchId != null) {
                var flow = db.flowLaunch().get(FlowLaunchId.of(flowLaunchId));
                var stage = ProtoMappers.toProtoStagesState(flow,
                        db.stageGroup()::findByIds,
                        db.flowLaunch()::findLaunchIds);
                return ProtoMappers.toProtoReleaseLaunch(launch, stage, false);
            } else {
                return ProtoMappers.toProtoReleaseLaunch(launch);
            }
        });
    }

    private StartConfig fillStartConfig(FrontendReleaseApi.StartReleaseRequest request) {
        CommitId commit;
        ArcBranch selectedBranch;

        if (ProtoMappers.hasOrderedArcRevision(request.getRevision())) {
            var revision = ProtoMappers.toOrderedArcRevision(request.getRevision());
            commit = revision.toRevision();
            selectedBranch = revision.getBranch();
        } else {
            commit = ProtoMappers.toArcRevision(request.getCommit());
            selectedBranch = ArcBranch.ofBranchName(request.getBranch());
        }

        OrderedArcRevision configRevision;
        if (ProtoMappers.hasOrderedArcRevision(request.getConfigRevision())) {
            configRevision = ProtoMappers.toOrderedArcRevision(request.getConfigRevision());
        } else {
            configRevision = null;
        }

        var fromLaunch = getLaunch(request);
        var launchReason = request.getLaunchReason().isEmpty() ? null : request.getLaunchReason();

        var config = new StartConfig(commit, selectedBranch, configRevision, fromLaunch, launchReason);
        log.info("Using config: {}", config);
        return config;
    }

    private LaunchId checkAccess(Common.ReleaseLaunchId releaseLaunchId, PermissionScope scope) {
        var launchId = ProtoMappers.toLaunchId(releaseLaunchId);
        var processId = launchId.getProcessId();

        db.currentOrReadOnly(() -> {
            var view = db.launches().getLaunchAccessView(launchId);
            var configRevision = view.getFlowInfo().getConfigRevision();
            permissionsService.checkAccess(AuthUtils.getUsername(), processId, configRevision, scope);
        });

        return launchId;
    }

    private static PermissionScope getStartReleaseScope(@Nullable FlowType customFlowType) {
        if (customFlowType == null) {
            return PermissionScope.START_FLOW;
        }
        return switch (customFlowType) {
            case FT_DEFAULT -> PermissionScope.START_FLOW;
            case FT_HOTFIX -> PermissionScope.START_HOTFIX;
            case FT_ROLLBACK -> PermissionScope.ROLLBACK_FLOW;
            case UNRECOGNIZED -> throw new IllegalStateException("Unsupported type: " + customFlowType);
        };
    }

    @Nullable
    private static LaunchId getLaunch(FrontendReleaseApi.StartReleaseRequest request) {
        if (request.hasRollbackLaunch()) {
            return getLaunch(request.getRollbackLaunch());
        }
        return null;
    }

    @Nullable
    private static LaunchId getLaunch(FrontendReleaseApi.GetReleaseFlowsRequest request) {
        if (request.hasRollbackLaunch()) {
            return getLaunch(request.getRollbackLaunch());
        }
        return null;
    }

    @Nullable
    private static LaunchId getLaunch(Common.ReleaseLaunchId releaseLaunchId) {
        if (releaseLaunchId.hasReleaseProcessId()) {
            var process = releaseLaunchId.getReleaseProcessId();
            if (!process.getDir().isEmpty() && !process.getId().isEmpty()) {
                return ProtoMappers.toLaunchId(releaseLaunchId);
            }
        }
        return null;
    }

    @Value
    private static class StartConfig {
        @Nonnull
        CommitId commit;
        @Nonnull
        ArcBranch selectedBranch;
        @Nullable
        OrderedArcRevision configRevision;
        @Nullable
        LaunchId rollbackUsingLaunch;
        @Nullable
        String launchReason;
    }
}
