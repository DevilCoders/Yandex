package ru.yandex.ci.api.controllers.frontend;

import java.util.List;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.collect.Iterables;
import com.google.protobuf.Int64Value;

import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.GetCommitsRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.GetCommitsResponse;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.lang.NonNullApi;

@NonNullApi
class ReleaseControllerTestHelper {

    private ReleaseControllerTestHelper() {
    }

    public static class Commits {
        public static final Common.Commit PROTO_R1 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_2);
        public static final Common.Commit PROTO_R1_1 = toProtoCommit(
                TestData.RELEASE_R2_1, TestData.RELEASE_BRANCH_COMMIT_2_1
        );
        public static final Common.Commit PROTO_R1_2 = toProtoCommit(
                TestData.RELEASE_R2_2, TestData.RELEASE_BRANCH_COMMIT_2_2
        );
        public static final Common.Commit PROTO_R5_1 = toProtoCommit(
                TestData.RELEASE_R6_1, TestData.RELEASE_BRANCH_COMMIT_6_1
        );
        public static final Common.Commit PROTO_R5_2 = toProtoCommit(
                TestData.RELEASE_R6_2, TestData.RELEASE_BRANCH_COMMIT_6_2
        );
        public static final Common.Commit PROTO_R5_3 = toProtoCommit(
                TestData.RELEASE_R6_3, TestData.RELEASE_BRANCH_COMMIT_6_3
        );
        public static final Common.Commit PROTO_R5_4 = toProtoCommit(
                TestData.RELEASE_R6_4, TestData.RELEASE_BRANCH_COMMIT_6_4
        );
        public static final Common.Commit PROTO_R2 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_3);
        public static final Common.Commit PROTO_R3 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_4);
        public static final Common.Commit PROTO_R4 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_5);
        public static final Common.Commit PROTO_R5 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_6);
        public static final Common.Commit PROTO_R6 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_7);
    }

    static GetCommitsResponse commitsResponse(int total, boolean hasMore, Common.Commit... commits) {
        var releaseCommits = Stream.of(commits)
                .map(c -> Common.ReleaseCommit.newBuilder()
                        .setManualDiscovery(Common.CommitDiscoveryState.newBuilder().setDiscovered(false))
                        .setCommit(c))
                .toArray(Common.ReleaseCommit.Builder[]::new);

        return commitsResponse(total, hasMore, releaseCommits);
    }

    static GetCommitsResponse commitsResponse(int total, boolean hasMore, Common.ReleaseCommit.Builder... commits) {
        return commitsResponse(total, hasMore, List.of(commits));
    }

    static GetCommitsResponse commitsResponse(int total, boolean hasMore, List<Common.ReleaseCommit.Builder> commits) {
        return commitsResponse(total, hasMore, commits, null);
    }

    static GetCommitsResponse commitsResponse(int total, boolean hasMore,
                                              List<Common.ReleaseCommit.Builder> commits,
                                              @Nullable Common.ReleaseCommit.Builder restartableCommit) {
        Common.Offset.Builder offsetBuilder = Common.Offset.newBuilder()
                .setHasMore(hasMore);

        if (total > 0) {
            offsetBuilder.setTotal(Int64Value.newBuilder().setValue(total).build());
        } else {
            offsetBuilder.setTotal(Int64Value.getDefaultInstance());
        }

        GetCommitsResponse.Builder builder = GetCommitsResponse.newBuilder()
                .setOffset(offsetBuilder);

        commits.forEach(builder::addReleaseCommits);

        if (commits.size() > 0 && hasMore) {
            Common.OrderedArcRevision lastRevision = Iterables.getLast(commits)
                    .getCommit().getRevision();

            builder.setNext(FrontendReleaseApi.CommitOffset.newBuilder()
                    .setBranch(lastRevision.getBranch())
                    .setNumber(lastRevision.getNumber())
                    .build());
        }


        if (restartableCommit != null) {
            builder.setRestartableReleaseCommit(restartableCommit);
        }

        return builder.build();
    }

    static Common.Commit toProtoTrunkCommit(ArcCommit commit) {
        return ProtoMappers.toProtoCommit(commit.toOrderedTrunkArcRevision(), commit);
    }

    static Common.Commit toProtoCommit(OrderedArcRevision arcRevision, ArcCommit commit) {
        return ProtoMappers.toProtoCommit(arcRevision, commit);
    }

    static Common.ReleaseCommit.Builder rc(Common.Commit commit) {
        return Common.ReleaseCommit.newBuilder()
                .setManualDiscovery(Common.CommitDiscoveryState.newBuilder().setDiscovered(false))
                .setCommit(commit);
    }

    static GetCommitsRequest forBranch(Branch branch) {
        return GetCommitsRequest.newBuilder()
                .setBranchCommits(branchId(branch))
                .build();
    }

    static Common.BranchId.Builder branchId(Branch branch) {
        return branchIn(branch.getProcessId(), branch.getId().getBranch());
    }

    static Common.BranchId.Builder branchIn(CiProcessId processId, String branch) {
        return Common.BranchId.newBuilder()
                .setBranch(branch)
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(processId));
    }

    static ArcCommit toCommit(OrderedArcRevision revision) {
        return TestData.toBranchCommit(revision, TestData.CI_USER);
    }

    static GetCommitsRequest freeCommitsRequest(CiProcessId processId, String branch) {
        return GetCommitsRequest.newBuilder()
                .setFreeCommits(branchIn(processId, branch))
                .build();
    }

    static GetCommitsRequest freeCommitsRequestWithLimit(CiProcessId processId, String branch, int limit) {
        return GetCommitsRequest.newBuilder()
                .setFreeCommits(branchIn(processId, branch))
                .setLimit(limit)
                .build();
    }

    static GetCommitsRequest freeCommitsRequestWithOffset(CiProcessId processId, String branch, long offsetNumber) {
        return freeCommitsRequestWithOffset(processId, branch, branch, offsetNumber);
    }

    static GetCommitsRequest freeCommitsRequestWithOffset(CiProcessId processId, String branch, String offsetBranch,
                                                          long offsetNumber) {
        return GetCommitsRequest.newBuilder()
                .setFreeCommits(branchIn(processId, branch))
                .setOffset(FrontendReleaseApi.CommitOffset.newBuilder()
                        .setBranch(offsetBranch)
                        .setNumber(offsetNumber)
                        .build()
                )
                .build();
    }

    static GetCommitsRequest freeCommitsRequestWithOffsetAndLimit(CiProcessId processId, String branch,
                                                                  int offsetNumber, int limit) {
        return GetCommitsRequest.newBuilder()
                .setFreeCommits(branchIn(processId, branch))
                .setOffset(FrontendReleaseApi.CommitOffset.newBuilder()
                        .setBranch(branch)
                        .setNumber(offsetNumber)
                        .build()
                )
                .setLimit(limit)
                .build();
    }

    static GetCommitsRequest freeCommitsRequest(CiProcessId processId, String branch,
                                                @Nullable FrontendReleaseApi.CommitOffset offset,
                                                @Nullable Integer limit) {
        var builder = GetCommitsRequest.newBuilder()
                .setFreeCommits(branchIn(processId, branch));
        if (offset != null) {
            builder.setOffset(offset);
        }
        if (limit != null) {
            builder.setLimit(limit);
        }
        return builder.build();
    }

}
