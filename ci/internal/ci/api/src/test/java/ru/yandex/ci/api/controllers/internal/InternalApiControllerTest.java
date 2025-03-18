package ru.yandex.ci.api.controllers.internal;

import java.time.Instant;
import java.util.Map;

import com.google.protobuf.TextFormat;
import io.grpc.ManagedChannel;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo;
import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.InternalApiGrpc;
import ru.yandex.ci.api.internal.InternalApiGrpc.InternalApiBlockingStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.launch.LaunchStateSynchronizer;
import ru.yandex.ci.engine.launch.OnCommitLaunchService.StartFlowParameters;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.job.CommitOffset;
import ru.yandex.ci.job.GetCommitsRequest;
import ru.yandex.ci.job.GetCommitsResponse;
import ru.yandex.ci.test.TestUtils;

import static java.util.Objects.requireNonNull;
import static org.assertj.core.api.Assertions.assertThat;

class InternalApiControllerTest extends ControllerTestBase<InternalApiBlockingStub> {

    private static final CiProcessId RELEASE_PROCESS_ID = TestData.SIMPLE_RELEASE_PROCESS_ID;

    @Autowired
    private LaunchStateSynchronizer launchStateSynchronizer;

    @Override
    protected InternalApiBlockingStub createStub(ManagedChannel channel) {
        return InternalApiGrpc.newBlockingStub(channel);
    }

    @Test
    void getCommitsForReleaseOffsetLimit() {
        mockValidationSuccessful();
        discoveryToR6();
        delegateToken(RELEASE_PROCESS_ID.getPath());
        startRelease(TestData.TRUNK_R3);
        var launchAtR5 = startRelease(TestData.TRUNK_R6);
        var flowIdAtR5 = requireNonNull(launchAtR5.getFlowLaunchId());

        var commits = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .setLimit(2)
                .build());

        assertThat(commits.getTimelineCommitsList())
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly("r6", "r5");

        assertThat(commits.getNext())
                .isEqualTo(CommitOffset.newBuilder()
                        .setBranch("trunk")
                        .setNumber(5)
                        .build()
                );

        commits = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .setLimit(2)
                .setOffset(commits.getNext())
                .build());

        assertThat(commits.getTimelineCommitsList())
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly("r4", "r3");

        assertThat(commits.getNext())
                .isEqualTo(CommitOffset.newBuilder()
                        .setBranch("trunk")
                        .setNumber(3)
                        .build()
                );

        commits = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .setLimit(2)
                .setOffset(commits.getNext())
                .build());

        assertThat(commits.getTimelineCommitsList())
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly("r2");

        assertThat(commits.hasNext()).isFalse();
    }

    @Test
    void getCommitsForRelease() {
        discoveryToR6();
        delegateToken(RELEASE_PROCESS_ID.getPath());
        var launchAtR2 = startRelease(TestData.TRUNK_R3);
        var launchAtR5 = startRelease(TestData.TRUNK_R6);
        var flowIdAtR5 = requireNonNull(launchAtR5.getFlowLaunchId());

        var ownCommits = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .setType(GetCommitsRequest.Type.FROM_PREVIOUS_ACTIVE)
                .build());

        assertThat(ownCommits.getCommitsList()).extracting(c -> c.getRevision().getHash())
                .containsExactly("r6", "r5", "r4");
        assertThat(ownCommits.getTimelineCommitsList()).extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly("r6", "r5", "r4");
        assertThat(ownCommits.getTimelineCommitsList()).extracting(c -> c.getRelease().getVersion().getFull())
                .containsOnly("2");


        var commitsWithNotReleasedYet = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .build());

        assertThat(commitsWithNotReleasedYet.getCommitsList())
                .extracting(c -> c.getRevision().getHash())
                .containsExactly("r6", "r5", "r4", "r3", "r2");

        assertThat(commitsWithNotReleasedYet.getTimelineCommitsList())
                .extracting(c -> c.getRelease().getVersion().getFull())
                .containsExactly("2", "2", "2", "1", "1");


        updateRelease(launchAtR2, LaunchState.Status.SUCCESS);


        commitsWithNotReleasedYet = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .build());

        assertThat(commitsWithNotReleasedYet.getCommitsList())
                .extracting(c -> c.getRevision().getHash())
                .containsExactly("r6", "r5", "r4");

        assertThat(commitsWithNotReleasedYet.getTimelineCommitsList())
                .extracting(c -> c.getRelease().getVersion().getFull())
                .containsExactly(
                        launchAtR5.getVersion().asString(),
                        launchAtR5.getVersion().asString(),
                        launchAtR5.getVersion().asString()
                );

        updateRelease(launchAtR5, LaunchState.Status.SUCCESS);

        var commitsForSuccess = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowIdAtR5)
                .build());

        assertThat(commitsForSuccess.getCommitsList())
                .extracting(c -> c.getRevision().getHash())
                .containsExactly("r6", "r5", "r4");

        assertThat(commitsForSuccess.getTimelineCommitsList())
                .extracting(c -> c.getRelease().getVersion().getFull())
                .containsExactly(
                        launchAtR5.getVersion().asString(),
                        launchAtR5.getVersion().asString(),
                        launchAtR5.getVersion().asString()
                );
    }

    @Test
    void getCommitsForReleaseInBranch() {
        var processId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR7();
        delegateToken(processId.getPath());

        // prepare two branches
        Branch branchR3 = createBranchAt(TestData.TRUNK_COMMIT_4, processId);
        Branch branchR5 = createBranchAt(TestData.TRUNK_COMMIT_6, processId);

        OrderedArcRevision revisionR3n1 = revision(1, branchR3.getArcBranch());
        OrderedArcRevision revisionR5n1 = revision(1, branchR5.getArcBranch());

        ArcCommit commitR3n1 = TestData.toBranchCommit(revisionR3n1);
        ArcCommit commitR5n1 = TestData.toBranchCommit(revisionR5n1);

        arcServiceStub.addCommit(commitR3n1, TestData.TRUNK_COMMIT_4,
                Map.of(processId.getPath().resolve("some.txt"), Repo.ChangelistResponse.ChangeType.Modify)
        );

        arcServiceStub.addCommit(commitR5n1, TestData.TRUNK_COMMIT_6,
                Map.of(processId.getPath().resolve("some.txt"), Repo.ChangelistResponse.ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(revisionR3n1.getBranch(), revisionR3n1.toRevision(), false);
        discoveryServicePostCommits.processPostCommit(revisionR5n1.getBranch(), revisionR5n1.toRevision(), false);

        delegateToken(processId.getPath(), branchR3.getArcBranch());
        delegateToken(processId.getPath(), branchR5.getArcBranch());

        // launch releases

        var launchR3n1 = startRelease(revisionR3n1, processId);
        var launchR5n1 = startRelease(revisionR5n1, processId);
        var r3n1Version = launchR3n1.getVersion().asString();
        var r5n1Version = launchR5n1.getVersion().asString();
        assertThat(r5n1Version).isNotEqualTo(r3n1Version).isNotBlank();
        assertThat(r3n1Version).isNotBlank();

        var launchR5n1Commits = getCommits(launchR5n1.getFlowLaunchId()).getTimelineCommitsList();
        assertThat(launchR5n1Commits)
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly(revisionR5n1.getCommitId(), "r6", "r5", "r4", "r3", "r2");

        assertThat(launchR5n1Commits)
                .extracting(c -> c.getRelease().getVersion().getFull())
                .containsExactly(r5n1Version, r5n1Version, r5n1Version, r3n1Version, r3n1Version, r3n1Version);

        assertThat(getCommits(launchR3n1.getFlowLaunchId()).getTimelineCommitsList())
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly(revisionR3n1.getCommitId(), "r4", "r3", "r2");

        updateRelease(launchR3n1, LaunchState.Status.SUCCESS);

        assertThat(getCommits(launchR5n1.getFlowLaunchId()).getTimelineCommitsList())
                .extracting(c -> c.getCommit().getRevision().getHash())
                .containsExactly(revisionR5n1.getCommitId(), "r6", "r5");
    }

    private GetCommitsResponse getCommits(String flowLaunchId) {
        return grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(flowLaunchId)
                .build());
    }

    @Test
    void getCommitsForAction() throws TextFormat.ParseException {
        discoveryToR6();
        delegateToken(TestData.SIMPLEST_FLOW_ID.getPath());
        var launch = onCommitLaunchService.startFlow(StartFlowParameters.builder()
                .processId(TestData.SIMPLEST_FLOW_ID)
                .branch(ArcBranch.trunk())
                .revision(TestData.TRUNK_R6.toRevision())
                .configOrderedRevision(TestData.TRUNK_R6)
                .triggeredBy(TestData.CI_USER)
                .build()
        );

        launch = mockFlowLaunchFor(launch);

        var commits = grpcService.getCommits(GetCommitsRequest.newBuilder()
                .setFlowLaunchId(requireNonNull(launch.getFlowLaunchId()))
                .setLimit(1)
                .build());

        assertThat(commits).isEqualTo(TestUtils.parseProtoTextFromString("""
                commits {
                  revision {
                    hash: "r6"
                    number: 6
                    pull_request_id: 96
                  }
                  date {
                    seconds: 1594676509
                    nanos: 42000000
                  }
                  message: "Message"
                  author: "andreevdm"
                }
                timeline_commits {
                  commit {
                    revision {
                      hash: "r6"
                      number: 6
                      pull_request_id: 96
                    }
                    date {
                      seconds: 1594676509
                      nanos: 42000000
                    }
                    message: "Message"
                    author: "andreevdm"
                  }
                }
                """, GetCommitsResponse.class));
    }

    private Launch startRelease(OrderedArcRevision revision) {
        return startRelease(revision, RELEASE_PROCESS_ID);
    }

    private Launch startRelease(OrderedArcRevision revision, CiProcessId processId) {
        var launch = launchService.startRelease(processId,
                revision.toRevision(),
                revision.getBranch(),
                TestData.CI_USER,
                null,
                false,
                false,
                null,
                true,
                null,
                null,
                null);

        return mockFlowLaunchFor(launch);
    }

    private void updateRelease(Launch launch, LaunchState.Status newStatus) {
        LaunchState state = new LaunchState(
                launch.getFlowLaunchId(), launch.getStarted(), Instant.now(clock), newStatus, ""
        );
        db.tx(() -> launchStateSynchronizer.stateUpdated(launch, state, false, null));
    }

    private Launch mockFlowLaunchFor(Launch launch) {
        return db.currentOrTx(() -> {
            var fl = flowLaunch(launch.getLaunchId());
            db.flowLaunch().save(fl);
            var updatedLaunch = launch.toBuilder().flowLaunchId(fl.getIdString()).build();
            db.launches().save(updatedLaunch);
            return updatedLaunch;
        });
    }

    private static FlowLaunchEntity flowLaunch(LaunchId launchId) {
        return FlowLaunchEntity.builder()
                .id(FlowLaunchId.of(launchId))
                .launchId(launchId)
                .flowInfo(FlowTestUtils.toFlowInfo(FlowFullId.of(launchId.getProcessId().getPath(), "flow"), null))
                .launchInfo(FlowTestUtils.SIMPLE_LAUNCH_INFO)
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .createdDate(Instant.now())
                .projectId("prj")
                .build();
    }

}
