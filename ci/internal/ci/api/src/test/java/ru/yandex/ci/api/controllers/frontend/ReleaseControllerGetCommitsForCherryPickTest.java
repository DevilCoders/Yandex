package ru.yandex.ci.api.controllers.frontend;

import io.grpc.ManagedChannel;
import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.CommitOffset;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc.ReleaseServiceBlockingStub;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.proto.ProtoMappers;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.commitsResponse;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.rc;

class ReleaseControllerGetCommitsForCherryPickTest extends ControllerTestBase<ReleaseServiceBlockingStub> {

    private static final boolean HAS_NO_MORE = false;
    private static final boolean HAS_MORE = true;

    private ArcBranch branch1;
    private ArcBranch branch2;
    private static final CiProcessId PROCESS_ID = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

    @BeforeEach
    void setUp() {
        discoveryToR7();
        delegateToken(PROCESS_ID.getPath());

        doReturn("releases/ci/release-component-1", "releases/ci/release-component-2")
                .when(branchNameGenerator)
                .generateName(any(), any(), anyInt());

        branch1 = createBranchAt(TestData.TRUNK_COMMIT_2, PROCESS_ID).getArcBranch();
        branch2 = createBranchAt(TestData.TRUNK_COMMIT_6, PROCESS_ID).getArcBranch();

        assertThat(branch1.asString()).isEqualTo("releases/ci/release-component-1");
        assertThat(branch2.asString()).isEqualTo("releases/ci/release-component-2");

        discoverCommits(branch1,
                TestData.RELEASE_R2_1,
                TestData.RELEASE_R2_2
        );

        discoverCommits(branch2,
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        );
    }


    @Override
    protected ReleaseServiceBlockingStub createStub(ManagedChannel channel) {
        return ReleaseServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void fromBranchToTrunk() {
        var trunk = ArcBranch.trunk();
        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk)))
                .isEqualTo(commitsResponse(4, HAS_NO_MORE,
                        Commits.PROTO_R5_4,
                        Commits.PROTO_R5_3,
                        Commits.PROTO_R5_2,
                        Commits.PROTO_R5_1
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk, 3)))
                .isEqualTo(commitsResponse(4, HAS_MORE,
                        Commits.PROTO_R5_4,
                        Commits.PROTO_R5_3,
                        Commits.PROTO_R5_2
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk, offset(TestData.RELEASE_R6_4), 2)))
                .isEqualTo(commitsResponse(4, HAS_MORE,
                        Commits.PROTO_R5_3,
                        Commits.PROTO_R5_2
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk, offset(TestData.RELEASE_R6_3), 2)))
                .isEqualTo(commitsResponse(2, HAS_NO_MORE,
                        Commits.PROTO_R5_2,
                        Commits.PROTO_R5_1
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk, offset(TestData.RELEASE_R6_2), 2)))
                .isEqualTo(commitsResponse(1, HAS_NO_MORE,
                        Commits.PROTO_R5_1
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, trunk, offset(TestData.RELEASE_R6_1), 2)))
                .isEqualTo(commitsResponse(0, HAS_NO_MORE, new Common.Commit[0]));

    }

    @Test
    void fromTrunkToBranch() {
        assertThat(grpcService.getCommitsToCherryPick(request(ArcBranch.trunk(), branch1)))
                .isEqualTo(commitsResponse(5, HAS_NO_MORE,
                        rc(Commits.PROTO_R6),
                        rc(PROTO_R5).addBranches(branch2.asString()),
                        rc(Commits.PROTO_R4),
                        rc(Commits.PROTO_R3),
                        rc(Commits.PROTO_R2)
                ));
    }

    @Test
    void fromBranchToBranch() {
        assertThat(grpcService.getCommitsToCherryPick(request(branch1, branch2)))
                .isEqualTo(commitsResponse(2, HAS_NO_MORE,
                        Commits.PROTO_R1_2,
                        Commits.PROTO_R1_1
                ));

        assertThat(grpcService.getCommitsToCherryPick(request(branch2, branch1)))
                .isEqualTo(commitsResponse(4, HAS_NO_MORE,
                        Commits.PROTO_R5_4,
                        Commits.PROTO_R5_3,
                        Commits.PROTO_R5_2,
                        Commits.PROTO_R5_1
                ));
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void sameBranch() {
        assertThatThrownBy(() -> grpcService.getCommitsToCherryPick(request(branch2, branch2)))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("FAILED_PRECONDITION: source and target branch equals: 'releases/ci/release-component-2'");
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void branchNotFound() {
        assertThatThrownBy(() -> grpcService.getCommitsToCherryPick(
                request(ArcBranch.trunk(), ArcBranch.ofBranchName("releases/ci/not-existant-branch")))
        )
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: branch 'releases/ci/not-existant-branch' not found");
    }

    //region Helpers

    private CommitOffset offset(OrderedArcRevision revision) {
        return CommitOffset.newBuilder()
                .setBranch(revision.getBranch().asString())
                .setNumber(revision.getNumber())
                .build();
    }

    private FrontendReleaseApi.GetCommitsToCherryPickRequest request(ArcBranch source, ArcBranch target) {
        return requestBuilder(source, target).build();
    }

    private FrontendReleaseApi.GetCommitsToCherryPickRequest request(ArcBranch source, ArcBranch target,
                                                                     CommitOffset offset, int limit) {
        return requestBuilder(source, target)
                .setOffset(offset)
                .setLimit(limit)
                .build();
    }

    private FrontendReleaseApi.GetCommitsToCherryPickRequest request(ArcBranch source, ArcBranch target,
                                                                     int limit) {
        return requestBuilder(source, target)
                .setLimit(limit)
                .build();
    }

    private FrontendReleaseApi.GetCommitsToCherryPickRequest.Builder requestBuilder(ArcBranch source,
                                                                                    ArcBranch target) {
        return FrontendReleaseApi.GetCommitsToCherryPickRequest.newBuilder()
                .setSourceBranch(source.asString())
                .setTargetBranch(Common.BranchId.newBuilder()
                        .setBranch(target.asString())
                        .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(PROCESS_ID))
                        .build());
    }
    //endregion

}
