package ru.yandex.ci.api.controllers.frontend;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import com.google.protobuf.StringValue;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import one.util.streamex.StreamEx;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetCommitsRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetCommitsResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetFlowLaunchResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetFlowLaunchesRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetFlowLaunchesResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetMetaForRunActionRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetMetaForRunActionResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.PinFlowLaunchRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.PinFlowLaunchResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SetTagsRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SetTagsResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.StartFlowRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.StartFlowResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestBranchesRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestBranchesResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestTagsRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.SuggestTagsResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.OnCommitFlowServiceGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcBranchCache;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitNotFoundException;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.EntityNotFoundException;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService.StartFlowParameters;
import ru.yandex.ci.engine.launch.ProjectNotInWhiteListException;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.TimelineCommit;
import ru.yandex.ci.util.OffsetResults;

@RequiredArgsConstructor
public class OnCommitFlowController extends OnCommitFlowServiceGrpc.OnCommitFlowServiceImplBase {

    private static final int GET_COMMITS_FOR_FLOW_START_MAX_LIMIT = 100;

    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final FlowLaunchApiService flowLaunchApiService;
    @Nonnull
    private final PermissionsService permissionsService;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final ArcBranchCache arcBranchCache;
    @Nonnull
    private final OnCommitLaunchService onCommitLaunchService;
    @Nonnull
    private final CommitFetchService commitFetchService;
    @Nonnull
    private final FlowVarsService flowVarsService;
    @Nonnull
    private final LaunchService launchService;

    @Override
    public void getFlowLaunches(
            GetFlowLaunchesRequest request,
            StreamObserver<GetFlowLaunchesResponse> responseObserver
    ) {
        var response = db.currentOrReadOnly(() -> {
            var launches = flowLaunchApiService.getFlowLaunchesByProcessId(
                    ProtoMappers.toCiProcessId(request.getFlowProcessId()),
                    ProtoMappers.toLaunchTableFilter(request),
                    request.getOffsetNumber(),
                    request.getLimit()
            );

            var prInfo = this.loadDiffSets(launches.items());
            var mappedLaunches = launches.items().stream()
                    .map(launch -> ProtoMappers.toProtoFlowLaunch(launch, prInfo.get(launch.getId())))
                    .toList();

            var builder = GetFlowLaunchesResponse.newBuilder()
                    .addAllLaunches(mappedLaunches)
                    .setOffset(ProtoMappers.toProtoOffset(launches));

            var commits = commitFetchService.getTimelineCommitsForActions(launches.items());
            var mappedCommits = launches.items().stream()
                    .map(Launch::getId)
                    .map(commits::get)
                    .map(TimelineCommit::getCommit)
                    .map(commit -> ProtoMappers.toProtoCommit(commit.getRevision(), commit.getCommit()))
                    .toList();

            Preconditions.checkState(mappedLaunches.size() == mappedCommits.size(),
                    "Internal error, mapped launches are commits does not match");
            builder.addAllCommits(mappedCommits);

            return builder.build();
        });

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getFlowLaunch(GetFlowLaunchRequest request,
                              StreamObserver<GetFlowLaunchResponse> responseObserver) {
        var response = db.currentOrReadOnly(() -> {
            var launchId = new LaunchId(
                    ProtoMappers.toCiProcessId(request.getFlowProcessId()),
                    request.getNumber()
            );
            var launch = db.launches().get(launchId);
            var diffSet = getDiffSet(launch);

            var builder = GetFlowLaunchResponse.newBuilder()
                    .setLaunch(ProtoMappers.toProtoFlowLaunch(launch, diffSet));

            var commits = commitFetchService.getTimelineCommitsForActions(List.of(launch));
            var commit = commits.get(launch.getId()).getCommit();
            builder.setCommit(ProtoMappers.toProtoCommit(commit.getRevision(), commit.getCommit()));

            return builder.build();
        });

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void suggestTags(SuggestTagsRequest request, StreamObserver<SuggestTagsResponse> responseObserver) {
        CiProcessId processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());

        OffsetResults<String> tags = OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        request.getLimit(),
                        limit -> db.launches().getTagsStartsWith(
                                processId, request.getTag(), request.getOffsetNumber(),
                                limit
                        )
                )
                .fetch();
        responseObserver.onNext(
                SuggestTagsResponse.newBuilder()
                        .addAllTags(tags.items())
                        .setOffset(ProtoMappers.toProtoOffset(tags))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void setTags(SetTagsRequest request,
                        StreamObserver<SetTagsResponse> responseObserver) {
        var launchId = checkAccess(request.getFlowProcessId(), request.getNumber(), PermissionScope.MODIFY);

        var tags = db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            return db.launches().save(launch.withTags(request.getTagsList())).getTags();
        });

        var response = SetTagsResponse.newBuilder().addAllTags(tags).build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void pinFlowLaunch(PinFlowLaunchRequest request,
                              StreamObserver<PinFlowLaunchResponse> responseObserver) {
        var launchId = checkAccess(request.getFlowProcessId(), request.getNumber(), PermissionScope.MODIFY);

        var response = db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            db.launches().save(launch.withPinned(request.getPin()));
            return PinFlowLaunchResponse.getDefaultInstance();
        });
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getCommits(GetCommitsRequest request,
                           StreamObserver<GetCommitsResponse> responseObserver) {
        var response = db.currentOrReadOnly(() -> {
            LaunchId launchId = new LaunchId(
                    ProtoMappers.toCiProcessId(request.getFlowProcessId()),
                    request.getNumber()
            );

            var commits = commitFetchService.fetchTimelineCommits(
                    launchId,
                    CommitFetchService.CommitOffset.empty(),
                    request.getLimit(),
                    false
            );
            var result = commits.mapItems(tc -> {
                var releaseCommit = tc.getCommit();
                return ProtoMappers.toProtoCommit(releaseCommit.getRevision(), releaseCommit.getCommit());
            });

            return FrontendOnCommitFlowLaunchApi.GetCommitsResponse.newBuilder()
                    .addAllCommits(result.items())
                    .setOffset(ProtoMappers.toProtoOffset(result))
                    .build();
        });
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void suggestBranches(SuggestBranchesRequest request,
                                StreamObserver<SuggestBranchesResponse> responseObserver) {
        CiProcessId processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());

        OffsetResults<String> branches = OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        request.getLimit(),
                        limit -> db.launches().getBranchesBySubstring(
                                processId, request.getBranch(), request.getOffset(), limit
                        )
                )
                .fetch();
        responseObserver.onNext(
                SuggestBranchesResponse.newBuilder()
                        .addAllBranches(branches.items())
                        .setOffset(ProtoMappers.toProtoOffset(branches))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void suggestBranchesForFlowStart(
            SuggestBranchesForFlowStartRequest request,
            StreamObserver<SuggestBranchesForFlowStartResponse> responseObserver
    ) {
        OffsetResults<String> branches = OffsetResults.builder()
                .withItems(
                        request.getLimit(),
                        limit -> {
                            if (request.getBranch().startsWith(ArcBranch.PR_BRANCH_PREFIX)) {
                                Long pullRequestIdPrefix = fetchPullRequestIdPrefix(request.getBranch());
                                return db.currentOrReadOnly(() ->
                                        db.pullRequestDiffSetTable().suggestPullRequestId(
                                                pullRequestIdPrefix, request.getOffset(), limit
                                        ))
                                        .stream()
                                        .map(ArcBranch::ofPullRequest)
                                        .map(ArcBranch::asString)
                                        .toList();
                            }

                            return arcBranchCache.suggestBranches(request.getBranch())
                                    .skip(request.getOffset())
                                    .limit(limit > 0 ? limit : Long.MAX_VALUE)
                                    .collect(Collectors.toList());
                        }
                )
                .fetch();

        responseObserver.onNext(
                SuggestBranchesForFlowStartResponse.newBuilder()
                        .addAllBranches(branches.items())
                        .setOffset(ProtoMappers.toProtoOffset(branches))
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void getCommitsForFlowStart(GetCommitsForFlowStartRequest request,
                                       StreamObserver<GetCommitsForFlowStartResponse> responseObserver) {
        int limitRequested = request.getLimit() > 0
                ? Math.min(request.getLimit(), GET_COMMITS_FOR_FLOW_START_MAX_LIMIT)
                : GET_COMMITS_FOR_FLOW_START_MAX_LIMIT;
        GetCommitsForFlowStartResponse response;

        switch (request.getStartRevisionCase()) {
            case BRANCH -> {
                ArcBranch branchRequested = ArcBranch.ofString(request.getBranch());
                if (branchRequested.isPr()) {
                    PullRequestDiffSet pullRequest = db.currentOrReadOnly(() ->
                            db.pullRequestDiffSetTable()
                                    .findLatestByPullRequestId(branchRequested.getPullRequestId())
                    ).orElseThrow(() -> GrpcUtils.notFoundException("Pull request not found " +
                            branchRequested.getPullRequestId()));
                    response = getPrCommitsForFlowStart(pullRequest, limitRequested);
                } else {
                    response = getBranchCommitsForFlowStart(branchRequested, limitRequested);
                }
            }
            case COMMIT_HASH_ID_PAIR -> {
                ArcRevision startAt = ArcRevision.of(request.getCommitHashIdPair().getStartAt());
                ArcRevision stopAt = request.getCommitHashIdPair().hasStopAt()
                        ? ArcRevision.of(request.getCommitHashIdPair().getStopAt().getValue())
                        : null;
                response = getRangeCommitsForFlowStart(startAt, stopAt, limitRequested);
            }
            case COMMIT -> response = getSingleCommit(ProtoMappers.toArcRevision(request.getCommit()));
            default -> throw GrpcUtils.invalidArgumentException("StartRevision is not set");
        }
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getMetaForRunAction(GetMetaForRunActionRequest request,
                                    StreamObserver<GetMetaForRunActionResponse> responseObserver) {
        var processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());
        var configRevision = ProtoMappers.toOrderedArcRevision(request.getConfigRevision());

        var flowDescription = onCommitLaunchService.getFlowDescription(processId, configRevision);
        responseObserver.onNext(GetMetaForRunActionResponse.newBuilder()
                .setFlow(flowDescription)
                .build());
        responseObserver.onCompleted();
    }

    @Override
    public void startFlow(StartFlowRequest request, StreamObserver<StartFlowResponse> responseObserver) {
        var flowVars = request.hasFlowVars()
                ? flowVarsService.parse(request.getFlowVars())
                : null;
        var response = startFlow(request, null, flowVars, null, null, false, LaunchMode.NORMAL);
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    private GetCommitsForFlowStartResponse getSingleCommit(ArcRevision revision) {
        var commit = arcService.getCommit(revision);

        OrderedArcRevision orderedArcRevision = toOrderedArcRevisionIfTrunk(commit);
        return GetCommitsForFlowStartResponse.newBuilder()
                .addAllCommits(List.of(
                        ProtoMappers.toProtoArcCommit(commit)
                ))
                .addAllCommitsWithNumber(List.of(
                        ProtoMappers.toProtoCommit(orderedArcRevision, commit)
                ))
                .build();
    }

    @Override
    public void cancelFlow(
            FrontendOnCommitFlowLaunchApi.CancelFlowRequest request,
            StreamObserver<FrontendOnCommitFlowLaunchApi.CancelFlowResponse> responseObserver
    ) {
        var launchId = checkAccess(request.getFlowProcessId(), request.getNumber(), PermissionScope.CANCEL_FLOW);
        var newLaunch = launchService.cancel(launchId, AuthUtils.getUsername(), "");

        responseObserver.onNext(
                FrontendOnCommitFlowLaunchApi.CancelFlowResponse.newBuilder()
                        .setLaunch(ProtoMappers.toProtoFlowLaunch(newLaunch, null))
                        .build()
        );
        responseObserver.onCompleted();
    }

    private GetCommitsForFlowStartResponse getRangeCommitsForFlowStart(
            ArcRevision startAt, @Nullable ArcRevision stopAt, int limit
    ) {
        var commits = arcService.getCommits(startAt, stopAt, limit + 1);
        var commitsAccordingToLimit = commits.stream().limit(limit).collect(Collectors.toList());

        var response = GetCommitsForFlowStartResponse.newBuilder()
                .addAllCommits(
                        commitsAccordingToLimit.stream()
                                .map(ProtoMappers::toProtoArcCommit)
                                .collect(Collectors.toList())
                )
                .addAllCommitsWithNumber(
                        commitsAccordingToLimit.stream()
                                .map(commit -> {
                                    var orderedArcRevision = toOrderedArcRevisionIfTrunk(commit);
                                    return ProtoMappers.toProtoCommit(orderedArcRevision, commit);
                                })
                                .collect(Collectors.toList())
                );

        boolean hasMore = commits.size() == limit + 1;
        if (hasMore) {
            ArcCommit nextCommit = commits.get(commits.size() - 1);
            var commitHashPairResponse = FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                    .setStartAt(nextCommit.getCommitId());
            if (stopAt != null) {
                commitHashPairResponse.setStopAt(StringValue.of(stopAt.getCommitId()));
            }
            response.setNextCommitIdPair(commitHashPairResponse);
        }
        return response.build();
    }

    private GetCommitsForFlowStartResponse getBranchCommitsForFlowStart(ArcBranch branchRequested, int limit) {
        var commits = arcService.getCommits(ArcRevision.of(branchRequested.asString()), null, limit + 1);
        var commitsAccordingToLimit = commits.stream().limit(limit).collect(Collectors.toList());
        var response = GetCommitsForFlowStartResponse.newBuilder()
                .addAllCommits(
                        commitsAccordingToLimit.stream()
                                .map(ProtoMappers::toProtoArcCommit)
                                .collect(Collectors.toList())
                )
                .addAllCommitsWithNumber(
                        commitsAccordingToLimit.stream()
                                .map(commit -> {
                                    var orderedArcRevision = toOrderedArcRevisionIfTrunk(commit);
                                    return ProtoMappers.toProtoCommit(orderedArcRevision, commit);
                                })
                                .collect(Collectors.toList())
                );

        boolean hasMore = commits.size() == limit + 1;
        if (hasMore) {
            ArcCommit nextCommit = commits.get(commits.size() - 1);
            response.setNextCommitIdPair(
                    FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                            .setStartAt(nextCommit.getCommitId())
            );
        }
        return response.build();
    }

    private GetCommitsForFlowStartResponse getPrCommitsForFlowStart(PullRequestDiffSet pullRequest, int limit) {
        PullRequestVcsInfo vcsInfo = pullRequest.getVcsInfo();
        ArcCommit mergeRevision = arcService.getCommit(vcsInfo.getMergeRevision())
                .withMessage(String.format(
                        "Merge %s into %s", vcsInfo.getFeatureRevision(), vcsInfo.getUpstreamRevision()
                ));

        List<ArcCommit> prCommits = arcService.getCommits(
                vcsInfo.getFeatureRevision(), vcsInfo.getUpstreamRevision(), limit
        );
        List<ArcCommit> commits = StreamEx.of(mergeRevision)
                .append(prCommits)
                .limit(limit)
                .collect(Collectors.toList());

        var response = GetCommitsForFlowStartResponse.newBuilder()
                .addAllCommits(
                        commits.stream().map(ProtoMappers::toProtoArcCommit).collect(Collectors.toList())
                )
                .addAllCommitsWithNumber(
                        commits.stream()
                                .map(commit -> {
                                    var orderedArcRevision = toOrderedArcRevisionIfTrunk(commit);
                                    return ProtoMappers.toProtoCommit(orderedArcRevision, commit);
                                })
                                .collect(Collectors.toList())
                );

        boolean hasMore = prCommits.size() == limit;
        if (hasMore) {
            ArcCommit nextCommit = prCommits.get(prCommits.size() - 1);
            response.setNextCommitIdPair(
                    FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                            .setStartAt(nextCommit.getCommitId())
                            .setStopAt(StringValue.of(
                                    vcsInfo.getUpstreamRevision().getCommitId()
                            ))
            );
        }

        return response.build();
    }

    // login - if not provided, current login will be used then access validated
    public StartFlowResponse startFlow(
            @Nonnull StartFlowRequest request,
            @Nullable String login,
            @Nullable JsonObject flowVars,
            @Nullable String title,
            @Nullable LaunchService.DelegatedSecurity delegatedSecurity,
            boolean skipUpdatingDiscoveredCommit,
            LaunchMode launchMode) {
        CiProcessId processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());
        if (login == null) {
            login = AuthUtils.getUsername();

            // TODO: Move permissions check out of this condition, after fixing autocheck/a.yaml
            permissionsService.checkAccess(login, processId, PermissionScope.START_FLOW);
        }

        ArcBranch branch = request.getBranch().isEmpty()
                ? ArcBranch.trunk()
                : ArcBranch.ofString(request.getBranch());

        ArcRevision revision = request.hasRevision()
                ? ProtoMappers.toArcRevision(request.getRevision())
                : arcService.getLastRevisionInBranch(branch);

        OrderedArcRevision configRevision = request.hasConfigRevision()
                && ProtoMappers.hasOrderedArcRevision(request.getConfigRevision())
                ? ProtoMappers.toOrderedArcRevision(request.getConfigRevision())
                : null;  // latest config revision

        if (branch.isUnknown()) {
            throw GrpcUtils.invalidArgumentException(
                    "Invalid branch %s, type is unknown".formatted(request.getBranch())
            );
        }

        Launch launch;
        try {
            var params = StartFlowParameters.builder()
                    .processId(processId)
                    .branch(branch)
                    .revision(revision)
                    .configOrderedRevision(configRevision)
                    .triggeredBy(login)
                    .launchMode(launchMode)
                    .notifyPullRequest(request.getNotifyPullRequest())
                    .flowVars(flowVars)
                    .flowTitle(title)
                    .delegatedSecurity(delegatedSecurity)
                    .skipUpdatingDiscoveredCommit(skipUpdatingDiscoveredCommit)
                    .build();

            launch = onCommitLaunchService.startFlow(params);
        } catch (EntityNotFoundException e) {
            throw GrpcUtils.notFoundException(e);
        } catch (IllegalArgumentException | CommitNotFoundException e) {
            throw GrpcUtils.invalidArgumentException(e);
        } catch (ProjectNotInWhiteListException e) {
            throw GrpcUtils.permissionDeniedException(e);
        }

        var diffSet = getDiffSet(launch);
        return StartFlowResponse.newBuilder()
                .setLaunch(ProtoMappers.toProtoFlowLaunch(launch, diffSet))
                .build();
    }

    @Nullable
    private PullRequestDiffSet getDiffSet(Launch launch) {
        var pullRequestInfo = launch.getVcsInfo().getPullRequestInfo();
        if (pullRequestInfo == null) {
            return null;
        }
        return db.currentOrReadOnly(() ->
                db.pullRequestDiffSetTable().findById(
                        pullRequestInfo.getPullRequestId(),
                        pullRequestInfo.getDiffSetId()
                )).orElse(null);
    }

    private LaunchId checkAccess(Common.FlowProcessId flowProcessId, int number, PermissionScope scope) {
        var processId = ProtoMappers.toCiProcessId(flowProcessId);
        var launchId = LaunchId.of(processId, number);

        db.currentOrReadOnly(() -> {
            var view = db.launches().getLaunchAccessView(launchId);
            var configRevision = view.getFlowInfo().getConfigRevision();
            permissionsService.checkAccess(AuthUtils.getUsername(), processId, configRevision, scope);
        });

        return launchId;
    }

    private Map<Launch.Id, PullRequestDiffSet> loadDiffSets(List<Launch> launches) {
        return db.currentOrReadOnly(() -> {
            var keys = new HashMap<PullRequestDiffSet.Id, Launch.Id>(launches.size());
            for (var launch : launches) {
                var pr = launch.getVcsInfo().getPullRequestInfo();
                if (pr != null) {
                    keys.put(PullRequestDiffSet.Id.of(pr.getPullRequestId(), pr.getDiffSetId()), launch.getId());
                }
            }

            var diffSets = db.pullRequestDiffSetTable().find(keys.keySet());
            var result = new HashMap<Launch.Id, PullRequestDiffSet>(diffSets.size());
            for (var diffSet : diffSets) {
                var launch = keys.get(diffSet.getId());
                Preconditions.checkState(launch != null,
                        "Internal error. Launch cannot be null for diff set %s", diffSet.getId());
                result.put(launch, diffSet);
            }
            return result;
        });
    }

    @Nullable
    private static Long fetchPullRequestIdPrefix(String branchPrefix) {
        Preconditions.checkArgument(branchPrefix.startsWith(ArcBranch.PR_BRANCH_PREFIX));

        if (branchPrefix.length() > ArcBranch.PR_BRANCH_PREFIX.length()) {
            ArcBranch arcBranch = ArcBranch.ofString(branchPrefix);
            Preconditions.checkArgument(arcBranch.isPr());

            return arcBranch.getPullRequestId();
        }
        return null;
    }

    private static OrderedArcRevision toOrderedArcRevisionIfTrunk(ArcCommit commit) {
        return commit.isTrunk()
                ? commit.toOrderedTrunkArcRevision()
                : commit.getRevision().toOrdered(ArcBranch.unknown(), 0, 0);
    }
}
