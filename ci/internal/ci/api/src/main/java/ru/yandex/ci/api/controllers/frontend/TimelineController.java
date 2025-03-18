package ru.yandex.ci.api.controllers.frontend;

import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.common.collect.Iterables;
import com.google.protobuf.Empty;
import io.grpc.Status;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.EntryStream;

import yandex.cloud.util.Strings;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.AddCommitRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetBranchesRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetBranchesResponse;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetCommitRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetCommitResponse;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetTimelineRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetTimelineResponse;
import ru.yandex.ci.api.internal.frontend.release.TimelineServiceGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitNotFoundException;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.discovery.ConfigChange;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.project.ProcessIdBranch;
import ru.yandex.ci.core.project.ProjectCounters;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineBranchItemByUpdateDate;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.notification.xiva.ReleasesTimelineChangedEvent;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.util.OffsetResults;

import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoBranch;
import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoBranchBuilder;
import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoLaunchStatusCounters;

@Slf4j
@RequiredArgsConstructor
public class TimelineController extends TimelineServiceGrpc.TimelineServiceImplBase {

    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final TimelineService timelineService;
    @Nonnull
    private final BranchService branchService;
    @Nonnull
    private final PermissionsService permissionsService;
    @Nonnull
    private final RevisionNumberService revisionNumberService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final XivaNotifier xivaNotifier;

    @Override
    public void getTimeline(GetTimelineRequest request,
                            StreamObserver<GetTimelineResponse> responseObserver) {
        CiProcessId processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        Offset offset = request.hasOffset()
                ? ProtoMappers.toTimelineOffset(request.getOffset())
                : Offset.EMPTY;

        ArcBranch branch = Strings.isNullOrEmpty(request.getBranch())
                ? ArcBranch.trunk()
                : ArcBranch.ofBranchName(request.getBranch());

        OffsetResults<TimelineItem> timeline = OffsetResults.builder()
                .withItems(
                        request.getLimit(),
                        limit -> timelineService.getTimeline(
                                processId,
                                branch,
                                offset,
                                limit
                        )
                )
                .fetch();

        var items = timeline.items();

        log.info("Fetched {} Timeline items", items.size());

        var stageMap = enrichTimelineItems(items);
        var lastBranchLaunchesMap = enrichTimelineBranches(items);

        List<Common.TimelineItem> protoItems = EntryStream.of(items)
                .mapKeyValue((i, item) -> {
                    boolean restartable = false;
                    if (i == 0) {
                        restartable = timelineService.canStartReleaseWithoutCommits(item, branch, false)
                                .isAllowed();
                    }
                    return ProtoMappers.toProtoTimelineItem(item, stageMap, lastBranchLaunchesMap, restartable);
                })
                .toList();

        GetTimelineResponse.Builder responseBuilder = GetTimelineResponse.newBuilder()
                .addAllItems(protoItems)
                .setOffset(ProtoMappers.toProtoOffset(timeline));

        if (!items.isEmpty()) {
            TimelineItem last = Iterables.getLast(items);
            responseBuilder.setNext(ProtoMappers.toProtoTimelineOffset(last.getNextStart()));
        }

        responseBuilder.setXivaSubscription(xivaNotifier.toXivaSubscription(
                new ReleasesTimelineChangedEvent(
                        processId, List.of(branch.getBranch())
                )
        ));

        responseObserver.onNext(responseBuilder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void getLastRelease(FrontendTimelineApi.GetLastReleaseRequest request,
                               StreamObserver<FrontendTimelineApi.GetLastReleaseResponse> responseObserver) {
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        var branch = Strings.isNullOrEmpty(request.getBranch())
                ? ArcBranch.trunk()
                : ArcBranch.ofBranchName(request.getBranch());

        var responseBuilder = FrontendTimelineApi.GetLastReleaseResponse.newBuilder();

        timelineService.getLastStarted(processId, branch)
                .map(item -> {
                    var stageMap = enrichTimelineItems(List.of(item));
                    var lastBranchLaunchesMap = enrichTimelineBranches(List.of(item));
                    var restartable = timelineService.canStartReleaseWithoutCommits(item, branch, false).isAllowed();
                    return ProtoMappers.toProtoTimelineItem(item, stageMap, lastBranchLaunchesMap, restartable);
                })
                .ifPresent(responseBuilder::setItem);

        responseObserver.onNext(responseBuilder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void createBranch(FrontendTimelineApi.CreateBranchRequest request,
                             StreamObserver<FrontendTimelineApi.CreateBranchResponse> responseObserver) {
        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        permissionsService.checkAccess(login, processId, PermissionScope.CREATE_BRANCH);

        var releaseState = db.currentOrReadOnly(() -> db.configStates().get(processId.getPath()))
                .getRelease(processId.getSubId());
        if (!releaseState.isReleaseBranchesEnabled()) {
            throw Status.INVALID_ARGUMENT.withDescription("Branches are not configured for release " + processId)
                    .asRuntimeException();
        }

        // assume commit is in trunk
        OrderedArcRevision revision = revisionNumberService.getOrderedArcRevision(
                ArcBranch.trunk(), ProtoMappers.toArcRevision(request.getCommit())
        );

        Branch branch = db.currentOrTx(() -> branchService.createBranch(processId, revision, login));

        responseObserver.onNext(
                FrontendTimelineApi.CreateBranchResponse.newBuilder()
                        .setBranch(toProtoBranch(branch))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void getBranches(GetBranchesRequest request,
                            StreamObserver<GetBranchesResponse> responseObserver) {
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());

        var offset = request.hasOffset()
                ? ProtoMappers.toBranchOffset(request.getOffset())
                : null;

        var branches = OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(request.getLimit(),
                        limit -> branchService.getBranches(processId, offset, limit)
                )
                .fetch();

        var processCounters = db.currentOrReadOnly(() -> {
            var project = db.configStates().get(processId.getPath()).getProject();
            Preconditions.checkState(project != null, "Project cannot be null for config %s", processId.getPath());
            return ProjectCounters.create(
                    db.launches().getActiveReleaseLaunchesCount(project, processId)
            );
        });


        var protoBranches = branches.items()
                .stream()
                .map(branch -> toProtoBranchBuilder(branch)
                        .addAllLaunchStatusCounter(
                                toProtoLaunchStatusCounters(
                                        processCounters,
                                        ProcessIdBranch.of(processId, branch.getId().getBranch())
                                )
                        )
                        .build()
                )
                .toList();
        var response = GetBranchesResponse.newBuilder()
                .addAllBranch(protoBranches)
                .setOffset(ProtoMappers.toProtoOffset(branches));

        if (branches.hasMore()) {
            var next = TimelineBranchItemByUpdateDate.Offset.of(
                    Iterables.getLast(branches.items()).getItem()
            );
            response.setNext(ProtoMappers.toProtoBranchOffset(next));
        }
        responseObserver.onNext(response.build());
        responseObserver.onCompleted();
    }

    @Override
    public void getBranch(FrontendTimelineApi.GetBranchRequest request,
                          StreamObserver<FrontendTimelineApi.GetBranchResponse> responseObserver) {
        CiProcessId processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        ArcBranch branch = ArcBranch.ofBranchName(request.getBranchName());

        Branch branchInfo = db.currentOrReadOnly(() -> branchService.getBranch(branch, processId));

        responseObserver.onNext(FrontendTimelineApi.GetBranchResponse.newBuilder()
                .setBranch(toProtoBranch(branchInfo))
                .build());
        responseObserver.onCompleted();
    }

    private Map<Launch.Id, Common.StagesState> enrichTimelineItems(List<TimelineItem> launches) {
        return enrichLaunches(db, launches.stream()
                .map(TimelineItem::getLaunch)
                .filter(Objects::nonNull)
                .collect(Collectors.toList()));
    }

    private Map<Launch.Id, Launch> enrichTimelineBranches(List<TimelineItem> branches) {
        var launchIds = branches.stream()
                .map(TimelineItem::getBranch)
                .filter(Objects::nonNull).filter(branch -> branch.getState().getLastLaunchNumber() > -1)
                .map(branch ->
                        Launch.Id.of(branch.getProcessId().asString(), branch.getState().getLastLaunchNumber())
                ).collect(Collectors.toSet());

        if (launchIds.isEmpty()) {
            return Map.of();
        }

        return db.currentOrReadOnly(() ->
                db.launches().find(launchIds).stream().collect(Collectors.toMap(Launch::getId, Function.identity()))
        );
    }

    static Map<Launch.Id, Common.StagesState> enrichLaunches(CiDb db, List<Launch> items) {
        return db.currentOrReadOnly(() -> {
            var launches = items.stream()
                    .filter(launch -> launch.getFlowLaunchId() != null)
                    .collect(Collectors.toMap(
                            Launch::getId,
                            launch -> FlowLaunchEntity.Id.of(launch.getFlowLaunchId())));

            var flowLaunches = db.flowLaunch().find(new HashSet<>(launches.values())).stream()
                    .collect(Collectors.toMap(FlowLaunchEntity::getId, Function.identity()));

            var stages = ProtoMappers.toProtoStagesStates(flowLaunches.values(),
                    db.stageGroup()::findByIds,
                    db.flowLaunch()::findLaunchIds);

            var result = new HashMap<Launch.Id, Common.StagesState>(launches.size());
            for (var e : launches.entrySet()) {
                var stage = stages.get(e.getValue());
                if (stage != null) {
                    result.put(e.getKey(), stage);
                }
            }

            return result;
        });
    }

    @Override
    public void addCommit(AddCommitRequest request, StreamObserver<Empty> responseObserver) {
        var login = AuthUtils.getUsername();
        var processId = ProtoMappers.toCiProcessId(request.getReleaseProcessId());
        permissionsService.checkAccess(login, processId, PermissionScope.ADD_COMMIT);

        ArcBranch branch = ArcBranch.ofBranchName(request.getBranch());
        if (!branch.isTrunk()) {
            throw GrpcUtils.invalidArgumentException("only 'trunk' branch is supported");
        }

        db.currentOrTx(() -> {
            ArcRevision revisionRequested = ProtoMappers.toArcRevision(request.getCommit());
            ArcCommit commitRequested = db.arcCommit().findOptional(revisionRequested.getCommitId())
                    .orElseThrow(() -> GrpcUtils.invalidArgumentException(
                            "not found commit '%s'".formatted(revisionRequested)
                    ));

            if (!branch.isTrunk()) {
                throw GrpcUtils.invalidArgumentException("only 'trunk' branch is supported");
            }

            OrderedArcRevision orderedRevisionRequested = revisionNumberService
                    .getOrderedArcRevision(branch, commitRequested);

            boolean alreadyDiscovered = db.discoveredCommit().findCommit(processId, orderedRevisionRequested)
                    .isPresent();
            if (alreadyDiscovered) {
                throw GrpcUtils.invalidArgumentException(
                        "commit '%s' already belongs to %s".formatted(revisionRequested, processId)
                );
            }

            OrderedArcRevision fromRevision = timelineService.getLastIn(processId, branch)
                    .map(TimelineItem::getArcRevision)
                    .orElse(null);

                /* fromRevision can be in base branch (e.g. trunk), from which branch was created, so
                     the comparison `fromRevision.isBefore(orderedRevisionRequested)` will throw an exception */
            Preconditions.checkArgument(branch.isTrunk());
            if (fromRevision != null && !fromRevision.isBefore(orderedRevisionRequested)) {
                throw GrpcUtils.invalidArgumentException(
                        "commit '%s' should be after '%s'".formatted(revisionRequested, fromRevision.getCommitId())
                );
            }
            db.discoveredCommit().updateOrCreate(
                    processId,
                    orderedRevisionRequested,
                    state -> state
                            .map(it -> it.toBuilder()
                                    .manualDiscovery(true)
                                    .manualDiscoveryBy(login)
                                    .manualDiscoveryAt(Instant.now())
                                    .build()
                            )
                            .orElseGet(() -> DiscoveredCommitState.builder()
                                    .configChange(new ConfigChange(ConfigChangeType.NONE))
                                    .manualDiscovery(true)
                                    .manualDiscoveryBy(login)
                                    .manualDiscoveryAt(Instant.now())
                                    .build()
                            )
            );
            log.info("commit {} was added to {} by user {}", revisionRequested, processId, login);
        });
        responseObserver.onNext(Empty.getDefaultInstance());
        responseObserver.onCompleted();
    }

    @Override
    public void getCommit(GetCommitRequest request,
                          StreamObserver<GetCommitResponse> responseObserver) {

        ArcRevision arcRevision = ProtoMappers.toArcRevision(request.getCommit());

        ArcCommit commit;
        try {
            commit = arcService.getCommit(arcRevision);
        } catch (CommitNotFoundException e) {
            throw GrpcUtils.notFoundException(e);
        }

        ArcBranch branch = ArcBranch.ofBranchName(request.getBranch());
        if (!branch.isTrunk() && !branch.isRelease()) {
            throw GrpcUtils.invalidArgumentException("only 'trunk' and release branches are supported");
        }
        OrderedArcRevision orderedArcRevision = revisionNumberService.getOrderedArcRevision(branch, commit);
        var response = GetCommitResponse.newBuilder()
                .setCommit(ProtoMappers.toProtoCommit(orderedArcRevision, commit))
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

}
