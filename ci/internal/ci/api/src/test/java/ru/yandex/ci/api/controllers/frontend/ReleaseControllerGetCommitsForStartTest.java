package ru.yandex.ci.api.controllers.frontend;

import java.util.List;
import java.util.Map;

import io.grpc.ManagedChannel;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo;
import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc.ReleaseServiceBlockingStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R1;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R2;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R3;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R4;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5_1;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5_2;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5_3;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R5_4;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.Commits.PROTO_R6;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.commitsResponse;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.forBranch;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequest;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithLimit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithOffset;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithOffsetAndLimit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.rc;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.toCommit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.toProtoCommit;

public class ReleaseControllerGetCommitsForStartTest extends ControllerTestBase<ReleaseServiceBlockingStub> {

    @Autowired
    private BranchService branchService;

    @Override
    protected ReleaseServiceBlockingStub createStub(ManagedChannel channel) {
        return ReleaseServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void withoutReleaseInBranch() {
        discoveryToR7();

        CiProcessId processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        delegateToken(processId.getPath());

        String branch = db.currentOrTx(() -> branchService.createBranch(
                processId, TestData.TRUNK_COMMIT_6.toOrderedTrunkArcRevision(),
                TestData.CI_USER
        )).getArcBranch().asString();

        List.of(
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        ).forEach(rev -> discoveryServicePostCommits.processPostCommit(rev.getBranch(), rev.toRevision(), false));

        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, branch))
        ).isEqualTo(
                commitsResponse(9, false,
                        rc(PROTO_R5_4), rc(PROTO_R5_3), rc(PROTO_R5_2), rc(PROTO_R5_1),
                        rc(PROTO_R5).addBranches(branch),
                        rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R2), rc(PROTO_R1)
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithLimit(processId, branch, 2))
        ).isEqualTo(
                commitsResponse(9, true,
                        PROTO_R5_4, PROTO_R5_3
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithOffset(processId, branch, 3))
        ).isEqualTo(
                commitsResponse(9, false,
                        rc(PROTO_R5_2), rc(PROTO_R5_1),
                        rc(PROTO_R5).addBranches(branch),
                        rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R2), rc(PROTO_R1)
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, ArcBranch.trunk().asString()))
        ).isEqualTo(
                commitsResponse(1, false,
                        PROTO_R6
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithOffsetAndLimit(processId, branch, 2, 2))
        ).isEqualTo(
                commitsResponse(9, true,
                        rc(PROTO_R5_1), rc(PROTO_R5).addBranches(branch)
                )
        );
    }

    @Test
    void inBranchWithReleaseAtTrunk() {
        discoveryToR7();

        CiProcessId processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        delegateToken(processId.getPath());

        launchService.startRelease(
                processId, TestData.TRUNK_R6.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);

        String branch = db.currentOrTx(() -> branchService.createBranch(
                processId, TestData.TRUNK_COMMIT_6.toOrderedTrunkArcRevision(),
                TestData.CI_USER
        )).getArcBranch().asString();

        List.of(
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        ).forEach(rev -> discoveryServicePostCommits.processPostCommit(rev.getBranch(), rev.toRevision(), false));


        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, branch))
        ).isEqualTo(
                commitsResponse(4, false,
                        // r2..r4 not discovered for processId
                        rc(PROTO_R5_4), rc(PROTO_R5_3), rc(PROTO_R5_2), rc(PROTO_R5_1)
                )
        );
    }

    @Test
    void getBranchCommits() {
        var releaseId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR7();

        Branch branchR3 = createBranch(releaseId, TestData.TRUNK_R4);
        String branchR3Name = branchR3.getArcBranch().asString();

        assertThat(grpcService.getCommits(forBranch(branchR3)))
                .describedAs("Should contain commits from base revision to first discovered")
                .isEqualTo(commitsResponse(3, false,
                        rc(PROTO_R3).addBranches(branchR3Name),
                        rc(PROTO_R2),
                        rc(PROTO_R1))
                );

        Branch branchR5 = db.currentOrTx(() -> createBranch(releaseId, TestData.TRUNK_R6));
        String branchR5Name = branchR5.getArcBranch().asString();

        assertThat(grpcService.getCommits(forBranch(branchR5)))
                .describedAs("Should contain commits up to next timeline item")
                .isEqualTo(commitsResponse(2, false,
                        rc(PROTO_R5).addBranches(branchR5Name),
                        rc(PROTO_R4))
                );

        OrderedArcRevision revisionR5n1 = revision(1, branchR5.getArcBranch());
        ArcCommit commitR5n1 = toCommit(revisionR5n1);
        arcServiceStub.addCommit(commitR5n1, TestData.TRUNK_COMMIT_6,
                Map.of(releaseId.getPath().resolve("some.txt"), Repo.ChangelistResponse.ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(revisionR5n1.getBranch(), revisionR5n1.toRevision(), false);

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithLimit(releaseId,
                        branchR5.getArcBranch().asString(), 2))
        )
                .describedAs("Free commits should return commits from trunk as well")
                .isEqualTo(commitsResponse(3, true,
                                rc(toProtoCommit(revisionR5n1, commitR5n1)),
                                rc(PROTO_R5).addBranches(branchR5Name)
                        )
                );

        assertThat(grpcService.getCommits(forBranch(branchR5)))
                .describedAs("Should contain only commits in trunk")
                .isEqualTo(commitsResponse(2, false, rc(PROTO_R5).addBranches(branchR5Name), rc(PROTO_R4)));
    }

    @Test
    void getBranchCommitsWithDifferentOffset() {
        var releaseId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR7();

        Branch branchR5 = createBranch(releaseId, TestData.TRUNK_R6);
        String branchR5Name = branchR5.getArcBranch().asString();

        OrderedArcRevision revisionR5n1 = revision(1, branchR5.getArcBranch());
        ArcCommit commitR5n1 = toCommit(revisionR5n1);
        arcServiceStub.addCommit(commitR5n1, TestData.TRUNK_COMMIT_6,
                Map.of(releaseId.getPath().resolve("some.txt"), Repo.ChangelistResponse.ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(revisionR5n1.getBranch(), revisionR5n1.toRevision(), false);

        assertThat(
                grpcService.getCommits(freeCommitsRequest(releaseId, branchR5.getArcBranch().asString()))
        )
                .describedAs("Should contain commits in branch and in trunk as well")
                .isEqualTo(commitsResponse(6, false,
                        rc(toProtoCommit(revisionR5n1, commitR5n1)),
                        rc(PROTO_R5).addBranches(branchR5Name),
                        rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R2), rc(PROTO_R1)
                ));

        assertThat(grpcService.getCommits(
                freeCommitsRequestWithOffset(
                        releaseId, branchR5.getArcBranch().asString(), "trunk", TestData.TRUNK_R4.getNumber()
                )
        ))
                .describedAs("Should contain commits not including supplied in offset")
                .isEqualTo(commitsResponse(6, false,
                        rc(PROTO_R2), rc(PROTO_R1)
                ));
    }

    @Test
    void startReleaseWithoutCommits_shouldReturnZeroCommits_whenLastReleaseIsNotInTerminalState() {
        CiProcessId processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        discoveryToR2();
        delegateToken(processId.getPath());

        launchService.startRelease(
                processId, TestData.TRUNK_R2.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);

        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, ArcBranch.trunk().asString()))
        ).isEqualTo(commitsResponse(0, false, List.of()));
    }

    @Test
    void startReleaseWithoutCommits() {
        CiProcessId processId = TestData.DUMMY_RELEASE_PROCESS_ID;
        discoveryToR3();
        delegateToken(processId.getPath());

        Launch launch = launchService.startRelease(
                processId, TestData.TRUNK_R2.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);
        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, ArcBranch.trunk().asString()))
        ).isEqualTo(
                commitsResponse(1, false,
                        List.of(
                                rc(toProtoCommit(TestData.TRUNK_R3, TestData.TRUNK_COMMIT_3))
                        ),
                        rc(toProtoCommit(TestData.TRUNK_R2, TestData.TRUNK_COMMIT_2))
                ));
    }

    @Test
    void startReleaseWithoutCommits_withLimitAndOffsetInRequest() {
        CiProcessId processId = TestData.DUMMY_RELEASE_PROCESS_ID;

        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        arcServiceStub.addCommit(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_1,
                Map.of(processId.getPath(), Repo.ChangelistResponse.ChangeType.Add));
        discovery(TestData.TRUNK_COMMIT_2);

        arcServiceStub.addCommit(TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_2,
                Map.of(processId.getPath(), Repo.ChangelistResponse.ChangeType.Modify));
        discovery(TestData.TRUNK_COMMIT_3);

        arcServiceStub.addCommit(TestData.TRUNK_COMMIT_4, TestData.TRUNK_COMMIT_3,
                Map.of(processId.getPath(), Repo.ChangelistResponse.ChangeType.Modify));
        discovery(TestData.TRUNK_COMMIT_4);

        delegateToken(processId.getPath());

        Launch launch = launchService.startRelease(
                processId, TestData.TRUNK_R2.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);
        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );

        var firstResponse = grpcService.getCommits(
                freeCommitsRequest(processId, ArcBranch.trunk().asString(), null, 1)
        );
        assertThat(firstResponse)
                .isEqualTo(commitsResponse(2, true,
                        rc(toProtoCommit(TestData.TRUNK_R4, TestData.TRUNK_COMMIT_4))
                ));

        var secondResponse = grpcService.getCommits(
                freeCommitsRequest(processId, ArcBranch.trunk().asString(), firstResponse.getNext(), 1)
        );
        assertThat(secondResponse).isEqualTo(commitsResponse(1, false,
                List.of(
                        rc(toProtoCommit(TestData.TRUNK_R3, TestData.TRUNK_COMMIT_3))
                ),
                rc(toProtoCommit(TestData.TRUNK_R2, TestData.TRUNK_COMMIT_2))
        ));
    }

    private Branch createBranch(CiProcessId releaseId, OrderedArcRevision revision) {
        delegateToken(releaseId.getPath());
        return db.currentOrTx(() -> branchService.createBranch(releaseId, revision, TestData.CI_USER));
    }

}
