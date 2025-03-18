package ru.yandex.ci.api.controllers.frontend;

import java.nio.file.Path;
import java.time.Duration;
import java.time.Instant;
import java.util.Map;
import java.util.Set;
import java.util.stream.Stream;

import com.google.protobuf.TextFormat;
import io.grpc.ManagedChannel;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo;
import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.flow.FlowServiceGrpc;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetBranchResponse;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetBranchesResponse;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetCommitRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi.GetCommitResponse;
import ru.yandex.ci.api.internal.frontend.release.TimelineServiceGrpc;
import ru.yandex.ci.api.internal.frontend.release.TimelineServiceGrpc.TimelineServiceBlockingStub;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;
import ru.yandex.ci.test.TestUtils;

import static com.google.protobuf.TextFormat.parse;
import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.assertj.core.api.InstanceOfAssertFactories.type;

class TimelineControllerTest extends ControllerTestBase<TimelineServiceBlockingStub> {

    @Autowired
    private TimelineService timelineService;

    @Autowired
    private BranchService branchService;

    @Override
    protected TimelineServiceBlockingStub createStub(ManagedChannel channel) {
        return TimelineServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void singleItem() {
        Launch launch = db.currentOrTx(() -> {
            Launch newLaunch = createLaunch(LaunchId.of(TestData.RELEASE_PROCESS_ID, 1), 78);
            db.launches().save(newLaunch);
            timelineService.updateTimelineLaunchItem(newLaunch);
            return newLaunch;
        });

        var processId = ProtoMappers.toProtoReleaseProcessId(TestData.RELEASE_PROCESS_ID);

        FrontendTimelineApi.GetTimelineResponse timeline = grpcService.getTimeline(
                FrontendTimelineApi.GetTimelineRequest.newBuilder()
                        .setReleaseProcessId(processId)
                        .build()
        );

        assertThat(timeline.getItemsList()).hasSize(1);
        Common.TimelineItem items = timeline.getItems(0);
        assertThat(items.hasTimelineBranch()).isFalse();
        assertThat(items.getRelease()).isEqualTo(ProtoMappers.toProtoReleaseLaunch(launch));

        var lastRelease = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .build());
        assertThat(lastRelease.hasItem()).isFalse(); // No running release so far
    }

    @SuppressWarnings("MethodLength")
    @Test
    void singleItemWithStages() {
        var launch0 = createLaunch(LaunchId.of(TestData.RELEASE_PROCESS_ID, 1), 77)
                .toBuilder()
                .started(clock.instant())
                .build();
        var launch1 = createLaunch(LaunchId.of(TestData.RELEASE_PROCESS_ID, 2), 78)
                .toBuilder()
                .started(clock.instant())
                .build();

        var lastRelease = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(TestData.RELEASE_PROCESS_ID))
                .build());
        assertThat(lastRelease.hasItem()).isFalse(); // No releases so far

        var flowLaunchId0 = FlowLaunchId.of(launch0.getFlowLaunchId());
        var flowLaunchId1 = FlowLaunchId.of(launch1.getFlowLaunchId());

        var flowProcessId = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "test").asString();
        var flowLaunch0 = FlowLaunchEntity.builder()
                .id(flowLaunchId0)
                .createdDate(Instant.now())
                .processId(flowProcessId)
                .projectId("projectid")
                .launchNumber(1)
                .stage(StoredStage.builder()
                        .id("stage-1")
                        .title("Stage 1")
                        .build())
                .stage(StoredStage.builder()
                        .id("stage-2")
                        .build())
                .stage(StoredStage.builder()
                        .id("stage-3")
                        .title("Stage 3")
                        .build())
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(launch0.getFlowInfo().getConfigRevision())
                        .flowId(launch0.getFlowInfo().getFlowId())
                        .runtimeInfo(launch0.getFlowInfo().getRuntimeInfo())
                        .stageGroupId("my-stages")
                        .build())
                .vcsInfo(launch0.getVcsInfo())
                .launchInfo(LaunchInfo.of("1"))
                .build();

        db.currentOrTx(() -> {
            db.launches().save(launch0);
            timelineService.updateTimelineLaunchItem(launch0);

            db.launches().save(launch1);
            timelineService.updateTimelineLaunchItem(launch1);

            var stageGroupState = StageGroupState.of("my-stages");
            stageGroupState.enqueue(new StageGroupState.QueueItem(flowLaunchId0, Set.of("stage-3"), null));
            stageGroupState.enqueue(new StageGroupState.QueueItem(flowLaunchId1,
                    Set.of("stage-1", "stage-2"), new StageGroupState.LockIntent("stage-3", Set.of("stage-1"))));
            db.stageGroup().save(stageGroupState);
            db.flowLaunch().save(flowLaunch0);

            var flowLaunch1 = FlowLaunchEntity.builder()
                    .id(flowLaunchId1)
                    .createdDate(Instant.now())
                    .processId(flowProcessId)
                    .projectId("projectid")
                    .launchNumber(2)
                    .stage(StoredStage.builder()
                            .id("stage-1")
                            .title("Stage 1")
                            .build())
                    .stage(StoredStage.builder()
                            .id("stage-2")
                            .build())
                    .stage(StoredStage.builder()
                            .id("stage-3")
                            .title("Stage 3")
                            .build())
                    .flowInfo(LaunchFlowInfo.builder()
                            .configRevision(launch1.getFlowInfo().getConfigRevision())
                            .flowId(launch1.getFlowInfo().getFlowId())
                            .runtimeInfo(launch1.getFlowInfo().getRuntimeInfo())
                            .stageGroupId("my-stages")
                            .build())
                    .vcsInfo(launch1.getVcsInfo())
                    .launchInfo(LaunchInfo.of("2"))
                    .build();
            db.flowLaunch().save(flowLaunch1);
        });


        var processId = ProtoMappers.toProtoReleaseProcessId(TestData.RELEASE_PROCESS_ID);
        FrontendTimelineApi.GetTimelineResponse timeline = grpcService.getTimeline(
                FrontendTimelineApi.GetTimelineRequest.newBuilder()
                        .setReleaseProcessId(processId)
                        .build()
        );

        assertThat(timeline.getItemsList()).hasSize(2);

        // Таймлайн отсортирован в обратну сторону
        Common.TimelineItem item0 = timeline.getItems(0);
        assertThat(item0.hasTimelineBranch()).isFalse();

        var expectLaunch = Common.ReleaseLaunchId.newBuilder()
                .setReleaseProcessId(Common.ReleaseProcessId.newBuilder()
                        .setDir("ci")
                        .setId("test")
                        .build())
                .setNumber(1)
                .build();
        var expectVersion = Common.Version.newBuilder()
                .setMajor("1")
                .setFull("1")
                .build();

        assertThat(item0.getRelease())
                .isEqualTo(ProtoMappers.toProtoReleaseLaunch(launch1,
                        Common.StagesState.newBuilder()
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-1")
                                        .setTitle("Stage 1")
                                        .setStatus(Common.StageState.StageStatus.ACQUIRED)
                                        .build())
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-2")
                                        .setTitle("No Title")
                                        .setStatus(Common.StageState.StageStatus.ACQUIRED)
                                        .build())
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-3")
                                        .setTitle("Stage 3")
                                        .setStatus(Common.StageState.StageStatus.NOT_ACQUIRED)
                                        .setBlockedByRelease(expectLaunch)
                                        .setBlockedByVersion(Common.BlockedBy.newBuilder()
                                                .setId(expectLaunch)
                                                .setVersion(expectVersion)
                                                .build())
                                        .build())
                                .build(), false));

        Common.TimelineItem item1 = timeline.getItems(1);
        assertThat(item1.hasTimelineBranch()).isFalse();

        assertThat(item1.getRelease())
                .isEqualTo(ProtoMappers.toProtoReleaseLaunch(launch0,
                        Common.StagesState.newBuilder()
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-1")
                                        .setTitle("Stage 1")
                                        .setStatus(Common.StageState.StageStatus.NOT_ACQUIRED)
                                        .build())
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-2")
                                        .setTitle("No Title")
                                        .setStatus(Common.StageState.StageStatus.NOT_ACQUIRED)
                                        .build())
                                .addStates(Common.StageState.newBuilder()
                                        .setId("stage-3")
                                        .setTitle("Stage 3")
                                        .setStatus(Common.StageState.StageStatus.ACQUIRED)
                                        .build())
                                .build(), false));

        lastRelease = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .build());
        assertThat(lastRelease.hasItem()).isFalse(); // Last release is same as current


        var flowController = FlowServiceGrpc.newBlockingStub(getChannel());
        var actualLaunchId = flowController.toLaunchId(FrontendFlowApi.ToLaunchIdRequest.newBuilder()
                .setFlowLaunchId(Common.FlowLaunchId.newBuilder()
                        .setId(flowLaunch0.getIdString())
                        .build())
                .build());
        var launchId = flowLaunch0.getLaunchId();
        var expectProcessId = Common.ProcessId.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(launchId.getProcessId()))
                .build();
        assertThat(actualLaunchId)
                .isEqualTo(FrontendFlowApi.ToLaunchIdResponse.newBuilder()
                        .setLaunchId(Common.LaunchId.newBuilder()
                                .setProcessId(expectProcessId)
                                .setNumber(launchId.getNumber())
                                .build())
                        .setProjectId("projectid")
                        .build());
    }

    @Test
    void getBranches() throws TextFormat.ParseException {
        mockValidationSuccessful();
        discoveryToR4();
        delegateToken(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID.getPath());

        db.currentOrTx(() -> {
            branchService.createBranch(
                    TestData.WITH_BRANCHES_RELEASE_PROCESS_ID,
                    TestData.TRUNK_R2,
                    TestData.CI_USER
            );
            clock.plus(Duration.ofHours(4));
            branchService.createBranch(
                    TestData.WITH_BRANCHES_RELEASE_PROCESS_ID,
                    TestData.TRUNK_R3,
                    TestData.CI_USER
            );
        });

        var top2 = grpcService.getBranches(
                FrontendTimelineApi.GetBranchesRequest.newBuilder()
                        .setReleaseProcessId(
                                ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID)
                        )
                        .setLimit(2)
                        .build()
        );

        assertThat(top2).isEqualTo(TestUtils.parseProtoTextFromString("""
                branch {
                  name: "releases/ci-test/test-sawmill-release-2"
                  created_by: "andreevdm"
                  created {
                    seconds: 1605112491
                  }
                  base_revision_hash: "r3"
                  trunk_commits_count: 2
                }
                branch {
                  name: "releases/ci-test/test-sawmill-release-1"
                  created_by: "andreevdm"
                  created {
                    seconds: 1605098091
                  }
                  base_revision_hash: "r2"
                  trunk_commits_count: 1
                }
                offset {
                }
                """, GetBranchesResponse.class));

        var top1 = grpcService.getBranches(
                FrontendTimelineApi.GetBranchesRequest.newBuilder()
                        .setReleaseProcessId(
                                ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID)
                        )
                        .setLimit(1)
                        .build()
        );

        assertThat(top1).isEqualTo(TestUtils.parseProtoTextFromString("""
                branch {
                  name: "releases/ci-test/test-sawmill-release-2"
                  created_by: "andreevdm"
                  created {
                    seconds: 1605112491
                  }
                  base_revision_hash: "r3"
                  trunk_commits_count: 2
                }
                next {
                  updated {
                    seconds: 1605112491
                  }
                  branch_name: "releases/ci-test/test-sawmill-release-2"
                }
                offset {
                  has_more: true
                }
                """, GetBranchesResponse.class));

        var next = grpcService.getBranches(
                FrontendTimelineApi.GetBranchesRequest.newBuilder()
                        .setReleaseProcessId(
                                ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID)
                        )
                        .setLimit(1)
                        .setOffset(top1.getNext())
                        .build()
        );

        assertThat(next).isEqualTo(TestUtils.parseProtoTextFromString("""
                branch {
                  name: "releases/ci-test/test-sawmill-release-1"
                  created_by: "andreevdm"
                  created {
                    seconds: 1605098091
                  }
                  base_revision_hash: "r2"
                  trunk_commits_count: 1
                }
                offset {
                }
                """, GetBranchesResponse.class));
    }

    @Test
    void getBranch() throws TextFormat.ParseException {
        discoveryToR4();
        delegateToken(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID.getPath());

        var branch = db.currentOrTx(() ->
                branchService.createBranch(
                        TestData.WITH_BRANCHES_RELEASE_PROCESS_ID,
                        TestData.TRUNK_R4,
                        TestData.CI_USER
                )
        );

        var branchResponse = grpcService.getBranch(FrontendTimelineApi.GetBranchRequest.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID))
                .setBranchName(branch.getArcBranch().asString())
                .build());

        assertThat(branchResponse).isEqualTo(parse("""
                branch {
                  name: "releases/ci-test/test-sawmill-release-1"
                  created_by: "andreevdm"
                  created {
                    seconds: 1605098091
                  }
                  base_revision_hash: "r4"
                  trunk_commits_count: 3
                }
                """, GetBranchResponse.class)
        );
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void addCommit() {
        discoveryToR8();
        delegateToken(TestData.SIMPLE_RELEASE_PROCESS_ID.getPath());

        db.currentOrReadOnly(() -> {
            assertThat(db.discoveredCommit().findCommit(TestData.SIMPLE_RELEASE_PROCESS_ID, TestData.TRUNK_R7))
                    .isPresent();
            assertThat(db.discoveredCommit().findCommit(TestData.SIMPLE_RELEASE_PROCESS_ID, TestData.TRUNK_R8))
                    .isEmpty();
        });

        var grpcRequest = FrontendTimelineApi.AddCommitRequest.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(TestData.SIMPLE_RELEASE_PROCESS_ID))
                .setBranch(ArcBranch.trunk().asString())
                .setCommit(Common.CommitId.newBuilder()
                        .setCommitId(TestData.TRUNK_R8.getCommitId())
                        .build())
                .build();

        assertThatThrownBy(() -> grpcService.addCommit(grpcRequest))
                .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [ADD_COMMIT], " +
                        "not in project [testenv]")
                .asInstanceOf(type(StatusRuntimeException.class))
                .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());

        abcServiceStub.addService(Abc.TE, TestData.USER42);
        grpcService.addCommit(grpcRequest);

        db.currentOrReadOnly(() -> {
            assertThat(db.discoveredCommit().findCommit(TestData.SIMPLE_RELEASE_PROCESS_ID, TestData.TRUNK_R8))
                    .isPresent();
        });
    }

    @Test
    void getCommit() throws TextFormat.ParseException {
        var request = GetCommitRequest.newBuilder()
                .setBranch(ArcBranch.trunk().asString())
                .setCommit(Common.CommitId.newBuilder()
                        .setCommitId(TestData.TRUNK_R2.getCommitId())
                        .build())
                .build();
        var response = grpcService.getCommit(request);
        assertThat(response).isEqualTo(parse("""
                commit {
                   revision {
                     hash: "r2"
                     branch: "trunk"
                     number: 2
                     pull_request_id: 92
                   }
                   date {
                     seconds: 1594676509
                     nanos: 42000000
                   }
                   message: "Message"
                   author: "sid-hugo"
                 }
                """, GetCommitResponse.class)
        );
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void getCommit_whenCommitIdNotFound() {
        var request = GetCommitRequest.newBuilder()
                .setCommit(Common.CommitId.newBuilder()
                        .setCommitId("not-found-commit")
                        .build())
                .build();
        assertThatThrownBy(() -> grpcService.getCommit(request))
                .asInstanceOf(type(StatusRuntimeException.class))
                .returns(Status.Code.NOT_FOUND, s -> s.getStatus().getCode());
    }

    @Test
    void getTimeline_branchWithLastLaunch() {
        discoveryToR4();
        delegateToken(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID.getPath());

        Launch launch = db.currentOrTx(() -> {
            Launch newLaunch = createLaunch(
                    LaunchId.of(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID, 1), (int) TestData.TRUNK_R4.getNumber()
            ).toBuilder()
                    .started(clock.instant())
                    .build();
            db.launches().save(newLaunch);
            timelineService.updateTimelineLaunchItem(newLaunch);
            return newLaunch;
        });

        var branch = db.currentOrTx(() ->
                branchService.createBranch(
                        TestData.WITH_BRANCHES_RELEASE_PROCESS_ID,
                        TestData.TRUNK_R4,
                        TestData.CI_USER
                )
        );

        var processId = ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID);

        FrontendTimelineApi.GetTimelineResponse timeline = grpcService.getTimeline(
                FrontendTimelineApi.GetTimelineRequest.newBuilder()
                        .setReleaseProcessId(processId).build()
        );

        assertThat(timeline.getItemsList()).hasSize(1);
        var item = timeline.getItemsList().get(0);

        assertThat(item.hasTimelineBranch()).isTrue();
        assertThat(item.hasRelease()).isFalse();
        assertThat(item.getTimelineBranch().getBranch()).isEqualTo(ProtoMappers.toProtoBranch(branch));
        assertThat(item.getTimelineBranch().getLastBranchRelease())
                .isEqualTo(ProtoMappers.toProtoReleaseLaunch(launch));
        assertThat(item.getShowInBranch()).isEqualTo("trunk");

        var timelineBranch = item.getTimelineBranch();

        var lastRelease = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .build());
        assertThat(lastRelease.getItem().getRelease())
                .isEqualTo(timelineBranch.getLastBranchRelease()); // Last release in branch - but we request from trunk
        assertThat(lastRelease.getItem().getShowInBranch())
                .isEqualTo(branch.getId().getBranch());

        var lastReleaseBranch = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .setBranch(branch.getId().getBranch())
                .build());
        assertThat(lastReleaseBranch.hasItem()).isFalse(); // Already latest release - on this branch

        var lastReleaseSomeBranch = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .setBranch("some/test/branch")
                .build());
        assertThat(lastReleaseSomeBranch.getItem().getRelease())
                .isEqualTo(timelineBranch.getLastBranchRelease()); // Reference to unknown branch
        assertThat(lastReleaseSomeBranch.getItem().getShowInBranch())
                .isEqualTo(branch.getId().getBranch());

    }

    @Test
    void getTimeline_shouldReturnRestartableFlag() {
        CiProcessId processId = TestData.DUMMY_RELEASE_PROCESS_ID;

        // prepare arc data
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

        // move firstLaunch, secondLaunch to final state
        var firstLaunch = launchService.startRelease(
                processId, TestData.TRUNK_R2.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);
        var secondLaunch = launchService.startRelease(
                processId, TestData.TRUNK_R3.toRevision(), ArcBranch.trunk(),
                TestData.CI_USER, null, false, false, null, true, null, null, null);

        db.currentOrTx(() ->
                Stream.of(firstLaunch, secondLaunch).forEach(launch -> {
                    var launchUpdated = db.launches().get(launch.getLaunchId()).toBuilder()
                            .status(LaunchState.Status.SUCCESS)
                            .finished(clock.instant())
                            .build();
                    db.launches().save(launchUpdated);
                    timelineService.updateTimelineLaunchItem(launchUpdated);
                }));

        var protoProcessId = ProtoMappers.toProtoReleaseProcessId(processId);

        FrontendTimelineApi.GetTimelineResponse timeline = grpcService.getTimeline(
                FrontendTimelineApi.GetTimelineRequest.newBuilder()
                        .setReleaseProcessId(protoProcessId)
                        .build()
        );

        assertThat(timeline.getItemsList()).hasSize(2);

        var topItem = timeline.getItemsList().get(0);
        var bottomItem = timeline.getItemsList().get(1);

        assertThat(topItem.getRelease().getId().getNumber()).isEqualTo(secondLaunch.getId().getLaunchNumber());
        assertThat(topItem.getRelease().getRestartable()).isTrue();

        assertThat(bottomItem.getRelease().getId().getNumber()).isEqualTo(firstLaunch.getId().getLaunchNumber());
        assertThat(bottomItem.getRelease().getRestartable()).isFalse();

        var lastRelease = grpcService.getLastRelease(FrontendTimelineApi.GetLastReleaseRequest.newBuilder()
                .setReleaseProcessId(protoProcessId)
                .build());
        assertThat(lastRelease.hasItem()).isFalse(); // Last release is exactly the same as last release in branch
    }

    @Test
    void getTimeline_shouldReturnXivaSubscription() {
        var processId = ProtoMappers.toProtoReleaseProcessId(TestData.RELEASE_PROCESS_ID);
        var timeline = grpcService.getTimeline(
                FrontendTimelineApi.GetTimelineRequest.newBuilder()
                        .setReleaseProcessId(processId)
                        .build()
        );
        assertThat(timeline.getXivaSubscription()).isEqualTo(Common.XivaSubscription.newBuilder()
                .setTopic("releases-timeline@a.2fb.2fc@my-release")
                .setService("ci-unit-test:trunk")
                .build());
    }

    private static Launch createLaunch(LaunchId launchId, int rev) {
        return Launch.builder()
                .title("Релиз #" + launchId.getNumber())
                .launchId(launchId)
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .created(TestData.COMMIT_DATE)
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(TestData.TRUNK_R4)
                        .flowId(FlowFullId.of(launchId.getProcessId().getPath(), launchId.getProcessId().getSubId()))
                        .stageGroupId("my-stages")
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                        .build())
                .flowLaunchId(FlowLaunchId.of(launchId).asString())
                .triggeredBy(TestData.CI_USER)
                .vcsInfo(LaunchVcsInfo.builder()
                        .revision(revision(rev))
                        .build()
                )
                .status(LaunchState.Status.RUNNING)
                .version(Version.major(String.valueOf(launchId.getNumber())))
                .build();
    }
}
