package ru.yandex.ci.api.controllers.frontend;

import java.nio.file.Path;
import java.time.Instant;
import java.util.List;

import com.google.gson.JsonParser;
import com.google.protobuf.Int64Value;
import com.google.protobuf.StringValue;
import io.grpc.ManagedChannel;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.flow.FlowServiceGrpc;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetMetaForRunActionRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetMetaForRunActionResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.OnCommitFlowServiceGrpc;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.OnCommitFlowServiceGrpc.OnCommitFlowServiceBlockingStub;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchUserData;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequest;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.assertj.core.api.InstanceOfAssertFactories.type;
import static ru.yandex.ci.core.test.TestData.TRUNK_COMMIT_R2R2;
import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoFlowProcessId;

public class OnCommitFlowControllerTest extends ControllerTestBase<OnCommitFlowServiceBlockingStub> {

    @Override
    protected OnCommitFlowServiceBlockingStub createStub(ManagedChannel channel) {
        return OnCommitFlowServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void getFlowLaunches() {
        initDiffSets();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        Launch launch = db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4)
        ));

        FrontendOnCommitFlowLaunchApi.GetFlowLaunchesResponse response =
                grpcService.getFlowLaunches(FrontendOnCommitFlowLaunchApi.GetFlowLaunchesRequest.newBuilder()
                        .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                .setId(processId.getSubId())
                                .setDir(processId.getDir())
                        )
                        .setLimit(50)
                        .build()
                );
        assertThat(launch.getStarted()).isNotNull();
        assertThat(launch.getFinished()).isNotNull();
        assertThat(launch.getFlowLaunchId()).isNotNull();

        var flowProcessId = toProtoFlowProcessId(processId);

        var expectBuilder = FrontendOnCommitFlowLaunchApi.GetFlowLaunchesResponse.newBuilder()
                .addAllLaunches(List.of(
                        FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder()
                                .setFlowProcessId(flowProcessId)
                                .setNumber(1)
                                .setTitle(launch.getTitle())
                                .setTriggeredBy(launch.getTriggeredBy())
                                .setRevisionHash(launch.getVcsInfo().getRevision().getCommitId())
                                .setBranch(ProtoMappers.toOnCommitFlowBranch(launch, TestData.DIFF_SET_1))
                                .setTriggeredBy(launch.getTriggeredBy())
                                .setCreated(ProtoConverter.convert(launch.getCreated()))
                                .setStarted(ProtoConverter.convert(launch.getStarted()))
                                .setFinished(ProtoConverter.convert(launch.getFinished()))
                                .setStatus(ProtoMappers.toProtoLaunchStatus(launch.getStatus()))
                                .setLaunchStatus(ProtoMappers.toProtoFlowLaunchStatus(launch.getStatus()))
                                .setStatusText(launch.getStatusText())
                                .setCancelable(!launch.getStatus().isTerminal())
                                .setPinned(true)
                                .setFlowLaunchId(Common.FlowLaunchId.newBuilder()
                                        .setId(launch.getFlowLaunchId())
                                )
                                .addAllTags(List.of("tag1"))
                                .setFlowDescription(
                                        Common.FlowDescription.newBuilder()
                                                .setFlowProcessId(flowProcessId)
                                )
                                .build()
                ))
                .setOffset(Common.Offset.newBuilder().setHasMore(false).build());

        expectBuilder.addCommitsBuilder()
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(launch.getVcsInfo().getRevision()))
                .setDate(ProtoConverter.convert(TestData.DS1_COMMIT.getCreateTime()))
                .setAuthor(TestData.DS1_COMMIT.getAuthor())
                .setMessage(TestData.DS1_COMMIT.getMessage());

        assertThat(response).isEqualTo(expectBuilder.build());
    }

    @Test
    void startFlowManuallySimple() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_IN_ROOT, "some-action");
        delegateToken(processId.getPath());

        var response = grpcService.startFlow(FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setBranch(ArcBranch.trunk().getBranch())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .setRevision(ProtoMappers.toCommitId(TestData.TRUNK_COMMIT_2))
                .build());

        assertThat(response.hasLaunch()).isTrue();
    }

    @Test
    void startFlowManuallyEmptyFlowVars() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_IN_ROOT, "some-action");
        delegateToken(processId.getPath());

        var response = grpcService.startFlow(FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setBranch(ArcBranch.trunk().getBranch())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .setRevision(ProtoMappers.toCommitId(TestData.TRUNK_COMMIT_2))
                .setFlowVars(Common.FlowVars.newBuilder().build())
                .build());

        assertThat(response.hasLaunch()).isTrue();
    }

    @Test
    void startFlowManualEmptyBranchAndRevision() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_IN_ROOT, "some-action");
        delegateToken(processId.getPath());

        var response = grpcService.startFlow(FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setFlowVars(Common.FlowVars.newBuilder().build())
                .build());

        assertThat(response.hasLaunch()).isTrue();
        var responseLaunch = response.getLaunch();
        assertThat(responseLaunch.getBranch().getName()).isEqualTo(ArcBranch.trunk().getBranch());
        assertThat(responseLaunch.getRevisionHash()).isEqualTo(TRUNK_COMMIT_R2R2.getCommitId());
    }

    @Test
    void startFlowManuallyEmptyFlowVarsNoRequired() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(Path.of("action/flow-vars-ui/a.yaml"), "with-flow-vars-ui");
        delegateToken(processId.getPath());

        assertThatThrownBy(() -> grpcService.startFlow(FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setBranch(ArcBranch.trunk().getBranch())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .setRevision(ProtoMappers.toCommitId(TestData.TRUNK_COMMIT_2))
                .setFlowVars(Common.FlowVars.newBuilder().build())
                .build())
        )
                .hasMessageContaining("flow vars ({}), passed from ui: (null) are not valid according to schema")
                .hasMessageContaining("object has missing required properties ([\\\"stage\\\",\\\"title\\\"])");
    }

    @Test
    void startFlowManuallyEmptyFlowVarsRequiredWithDefault() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(Path.of("action/flow-vars-ui/a.yaml"), "with-required-and-default");
        delegateToken(processId.getPath());

        var response = grpcService.startFlow(FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setBranch(ArcBranch.trunk().getBranch())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .setRevision(ProtoMappers.toCommitId(TestData.TRUNK_COMMIT_2))
                .setFlowVars(Common.FlowVars.newBuilder().build())
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var launch = db.currentOrTx(() -> db.launches().get(launchId));
        assertThat(launch.getFlowInfo().getFlowVars())
                .isNotNull()
                .extracting(JobResource::getData)
                .isEqualTo(JsonParser.parseString("""
                        {"state":"testing"}
                        """));
    }

    @Test
    void getFlowDescription() {
        discoveryToR2();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_IN_ROOT, "some-action");
        delegateToken(processId.getPath());

        var response = grpcService.getMetaForRunAction(GetMetaForRunActionRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .build());


        var expectedResponse = TestUtils.parseProtoText(
                "action/getMetaForRunAction.pb",
                GetMetaForRunActionResponse.class
        );
        assertThat(response).isEqualTo(expectedResponse);
    }

    @Test
    void getFlowLaunch() {
        initDiffSets();
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        Launch launch = db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4)
        ));

        FrontendOnCommitFlowLaunchApi.GetFlowLaunchResponse response =
                grpcService.getFlowLaunch(FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setNumber(1)
                        .build()
                );

        assertThat(launch.getStarted()).isNotNull();
        assertThat(launch.getFinished()).isNotNull();
        assertThat(launch.getFlowLaunchId()).isNotNull();

        var flowProcessId = toProtoFlowProcessId(processId);

        var expectBuilder = FrontendOnCommitFlowLaunchApi.GetFlowLaunchResponse.newBuilder()
                .setLaunch(
                        FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder()
                                .setFlowProcessId(flowProcessId)
                                .setNumber(1)
                                .setTitle(launch.getTitle())
                                .setTriggeredBy(launch.getTriggeredBy())
                                .setRevisionHash(launch.getVcsInfo().getRevision().getCommitId())
                                .setBranch(ProtoMappers.toOnCommitFlowBranch(launch, TestData.DIFF_SET_1))
                                .setTriggeredBy(launch.getTriggeredBy())
                                .setCreated(ProtoConverter.convert(launch.getCreated()))
                                .setStarted(ProtoConverter.convert(launch.getStarted()))
                                .setFinished(ProtoConverter.convert(launch.getFinished()))
                                .setStatus(ProtoMappers.toProtoLaunchStatus(launch.getStatus()))
                                .setLaunchStatus(ProtoMappers.toProtoFlowLaunchStatus(launch.getStatus()))
                                .setStatusText(launch.getStatusText())
                                .setCancelable(!launch.getStatus().isTerminal())
                                .setPinned(true)
                                .addAllTags(List.of("tag1"))
                                .setFlowLaunchId(Common.FlowLaunchId.newBuilder()
                                        .setId(launch.getFlowLaunchId())
                                )
                                .setFlowDescription(
                                        Common.FlowDescription.newBuilder()
                                                .setFlowProcessId(flowProcessId)
                                )
                                .build()
                );

        expectBuilder.getCommitBuilder()
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(launch.getVcsInfo().getRevision()))
                .setDate(ProtoConverter.convert(TestData.DS1_COMMIT.getCreateTime()))
                .setAuthor(TestData.DS1_COMMIT.getAuthor())
                .setMessage(TestData.DS1_COMMIT.getMessage());

        assertThat(response).isEqualTo(expectBuilder.build());
    }

    @Test
    void suggestTags() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4).toBuilder()
                        .tag("tag2")
                        .build()
        ));
        db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 2), TestData.TRUNK_R4).toBuilder()
                        .tag("tag22")
                        .tag("tag23")
                        .build()
        ));

        FrontendOnCommitFlowLaunchApi.SuggestTagsResponse response =
                grpcService.suggestTags(FrontendOnCommitFlowLaunchApi.SuggestTagsRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setTag("tag2")
                        .setLimit(2)
                        .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestTagsResponse.newBuilder()
                        .addAllTags(List.of("tag2", "tag22"))
                        .setOffset(Common.Offset.newBuilder().setHasMore(true).build())
                        .build()
        );
    }

    @Test
    void setTags() {
        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_ABC);
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        Launch launch = db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R2)
        ));
        FrontendOnCommitFlowLaunchApi.SetTagsResponse response = grpcService.setTags(
                FrontendOnCommitFlowLaunchApi.SetTagsRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setNumber(1)
                        .addAllTags(List.of("tag1", "tag2"))
                        .build()
        );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SetTagsResponse.newBuilder()
                        .addAllTags(List.of("tag1", "tag2"))
                        .build()
        );
        db.currentOrTx(() ->
                assertThat(db.launches().get(launch.getLaunchId()))
                        .isEqualTo(
                                launch.toBuilder()
                                        .clearTags()
                                        .tags(List.of("tag1", "tag2"))
                                        .build()
                        )
        );
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void setTags_whenFlowLaunchNotFound() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        assertThatThrownBy(() ->
                grpcService.setTags(
                        FrontendOnCommitFlowLaunchApi.SetTagsRequest.newBuilder()
                                .setFlowProcessId(toProtoFlowProcessId(processId))
                                .setNumber(1)
                                .addAllTags(List.of("tag1"))
                                .build()
                ))
                .hasMessage("NOT_FOUND: Unable to find key [%s] in table [%s]".formatted(
                        "Launch.Id(processId=a/b/c/a.yaml:f:sawmill, launchNumber=1)", "main/Launch"))
                .asInstanceOf(type(StatusRuntimeException.class))
                .returns(Status.Code.NOT_FOUND, s -> s.getStatus().getCode());
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void setTags_whenAccessDenied() {
        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_ABC);

        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        abcServiceStub.addService(Abc.CI, "CI", TestData.CI_USER); // remove User42 from access list
        assertThatThrownBy(() ->
                grpcService.setTags(
                        FrontendOnCommitFlowLaunchApi.SetTagsRequest.newBuilder()
                                .setFlowProcessId(toProtoFlowProcessId(processId))
                                .setNumber(1)
                                .addAllTags(List.of("tag1"))
                                .build()
                ))
                .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [MODIFY], not in project [ci]")
                .asInstanceOf(type(StatusRuntimeException.class))
                .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
    }

    @Test
    void pinFlowLaunch() {
        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_ABC);
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        Launch launch = db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R2)
        ));
        FrontendOnCommitFlowLaunchApi.PinFlowLaunchResponse response = grpcService.pinFlowLaunch(
                FrontendOnCommitFlowLaunchApi.PinFlowLaunchRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setNumber(1)
                        .setPin(true)
                        .build()
        );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.PinFlowLaunchResponse.newBuilder()
                        .build()
        );
        db.currentOrTx(() ->
                assertThat(db.launches().get(launch.getLaunchId()))
                        .isEqualTo(
                                launch.toBuilder()
                                        .pinned(true)
                                        .build()
                        )
        );
    }

    @Test
    @SuppressWarnings("ResultOfMethodCallIgnored")
    void pinFlowLaunch_whenAccessDenied() {
        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_ABC);

        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        abcServiceStub.addService(Abc.CI, "CI", TestData.CI_USER); // remove User42 from access list
        assertThatThrownBy(() ->
                grpcService.pinFlowLaunch(
                        FrontendOnCommitFlowLaunchApi.PinFlowLaunchRequest.newBuilder()
                                .setFlowProcessId(toProtoFlowProcessId(processId))
                                .setNumber(1)
                                .setPin(true)
                                .build()
                ))
                .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [MODIFY], not in project [ci]")
                .asInstanceOf(type(StatusRuntimeException.class))
                .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void cancelsFlow() {
        discoveryToR2();
        var processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        var launchId = LaunchId.of(processId, 1);
        db.currentOrTx(() -> {
            db.launches().save(
                    createLaunchOnPr(launchId, TestData.TRUNK_R2).toBuilder()
                            .status(LaunchState.Status.RUNNING)
                            .build()
            );
            db.flowLaunch().save(flowLaunch(launchId));
        });

        var cancelRequest = FrontendOnCommitFlowLaunchApi.CancelFlowRequest.newBuilder()
                .setFlowProcessId(toProtoFlowProcessId(processId))
                .setNumber(launchId.getNumber())
                .build();
        assertThatThrownBy(() -> grpcService.cancelFlow(cancelRequest))
                .hasMessage("INTERNAL: Cannot check permission scope for config %s on revision %s with status %s"
                        .formatted(processId.getPath(), TestData.TRUNK_R2, ConfigStatus.SECURITY_PROBLEM));

        delegateToken(TestData.CONFIG_PATH_ABC);
        var launch = grpcService.cancelFlow(cancelRequest).getLaunch();

        assertThat(launch.getStatus()).isEqualTo(Common.LaunchStatus.CANCELLING);


        var flowController = FlowServiceGrpc.newBlockingStub(getChannel());
        var actualLaunchId = flowController.toLaunchId(FrontendFlowApi.ToLaunchIdRequest.newBuilder()
                .setFlowLaunchId(Common.FlowLaunchId.newBuilder()
                        .setId(FlowLaunchId.of(launchId).asString())
                        .build())
                .build());

        var expectProcessId = Common.ProcessId.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .build();
        assertThat(actualLaunchId)
                .isEqualTo(FrontendFlowApi.ToLaunchIdResponse.newBuilder()
                        .setLaunchId(Common.LaunchId.newBuilder()
                                .setProcessId(expectProcessId)
                                .setNumber(launchId.getNumber())
                                .build())
                        .setProjectId("prj")
                        .build());
    }

    @Test
    void getCommits_whenLaunchWasOnPr() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        Launch launch = db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4)
        ));
        FrontendOnCommitFlowLaunchApi.GetCommitsResponse response = grpcService.getCommits(
                FrontendOnCommitFlowLaunchApi.GetCommitsRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setNumber(1)
                        .setLimit(10)
                        .build()
        );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.GetCommitsResponse.newBuilder()
                        .addAllCommits(List.of(
                                Common.Commit.newBuilder()
                                        .setRevision(
                                                Common.OrderedArcRevision.newBuilder()
                                                        .setHash(TestData.DS1_COMMIT.getCommitId()) // ds1 is a merge
                                                        // revision
                                                        .setBranch("pr:42")
                                                        .setNumber(0)
                                                        .build()
                                        )
                                        .setAuthor(TestData.DS1_COMMIT.getAuthor())
                                        .setDate(ProtoConverter.convert(
                                                TestData.DS1_COMMIT.getCreateTime()
                                        ))
                                        .setMessage(TestData.DS1_COMMIT.getMessage())
                                        .build()
                        ))
                        .setOffset(Common.Offset.newBuilder().setTotal(Int64Value.of(1)).build())
                        .build()
        );
        db.currentOrTx(() ->
                assertThat(db.launches().get(launch.getLaunchId()))
                        .isEqualTo(
                                launch.toBuilder()
                                        .pinned(true)
                                        .build()
                        )
        );
    }

    @Test
    void suggestBranches() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4)
        ));

        FrontendOnCommitFlowLaunchApi.SuggestBranchesResponse response =
                grpcService.suggestBranches(FrontendOnCommitFlowLaunchApi.SuggestBranchesRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setBranch("")
                        .setLimit(10)
                        .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesResponse.newBuilder()
                        .addAllBranches(List.of("pr:42"))
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );
    }

    @Test
    void suggestBranchesWithOffset() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        db.currentOrTx(() -> db.launches().save(
                createLaunchOnPr(LaunchId.of(processId, 1), TestData.TRUNK_R4)
        ));

        FrontendOnCommitFlowLaunchApi.SuggestBranchesResponse response =
                grpcService.suggestBranches(FrontendOnCommitFlowLaunchApi.SuggestBranchesRequest.newBuilder()
                        .setFlowProcessId(toProtoFlowProcessId(processId))
                        .setBranch("")
                        .setOffset(1)
                        .setLimit(10)
                        .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesResponse.newBuilder()
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );
    }

    @Test
    void suggestBranchesForFlowStart_whenPrefixIsReleaseBranch() {
        discoveryToR2();

        FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse response =
                grpcService.suggestBranchesForFlowStart(
                        FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartRequest.newBuilder()
                                .setBranch("releases/")
                                .setLimit(10)
                                .setOffset(1)
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse.newBuilder()
                        .addAllBranches(List.of("releases/ci/release-component-2"))
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );

        // there is no limit and offset in request
        response = grpcService.suggestBranchesForFlowStart(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartRequest.newBuilder()
                        .setBranch("releases/")
                        .build()
        );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse.newBuilder()
                        .addAllBranches(List.of(
                                "releases/ci/release-component-1",
                                "releases/ci/release-component-2"
                        ))
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );
    }

    @Test
    void suggestBranchesForFlowStart_whenPrefixIsPrBranch() {
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(
                createPullRequestDiffSet(42)
        ));

        FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse response =
                grpcService.suggestBranchesForFlowStart(
                        FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartRequest.newBuilder()
                                .setBranch("pr:4")
                                .setLimit(10)
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse.newBuilder()
                        .addAllBranches(List.of("pr:42"))
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );

        response = grpcService.suggestBranchesForFlowStart(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartRequest.newBuilder()
                        .setBranch("pr:")
                        .setLimit(10)
                        .build()
        );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.SuggestBranchesForFlowStartResponse.newBuilder()
                        .addAllBranches(List.of("pr:42"))
                        .setOffset(Common.Offset.newBuilder().build())
                        .build()
        );
    }

    @Test
    void getCommitsForFlowStart_whenRequestedCommitsOfPr() {
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(
                createPullRequestDiffSet(42)
        ));

        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse response =
                grpcService.getCommitsForFlowStart(
                        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartRequest.newBuilder()
                                .setBranch("pr:42")
                                .setLimit(2)
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse.newBuilder()
                        .addAllCommits(
                                List.of(
                                        Common.ArcCommit.newBuilder()
                                                .setHash(TestData.DS3_COMMIT.getCommitId())
                                                .setDate(ProtoConverter.convert(
                                                        TestData.DS3_COMMIT.getCreateTime()
                                                ))
                                                .setAuthor(TestData.DS3_COMMIT.getAuthor())
                                                .setMessage("Merge ds3 into r2")
                                                .build(),
                                        Common.ArcCommit.newBuilder()
                                                .setHash(TestData.DS3_COMMIT.getCommitId())
                                                .setDate(ProtoConverter.convert(
                                                        TestData.DS3_COMMIT.getCreateTime()
                                                ))
                                                .setAuthor(TestData.DS3_COMMIT.getAuthor())
                                                .setMessage(TestData.DS3_COMMIT.getMessage())
                                                .build()
                                )
                        )
                        .addAllCommitsWithNumber(
                                List.of(
                                        Common.Commit.newBuilder()
                                                .setRevision(
                                                        Common.OrderedArcRevision.newBuilder()
                                                                .setHash(TestData.DS3_COMMIT.getCommitId())
                                                                .build()
                                                )
                                                .setDate(ProtoConverter.convert(
                                                        TestData.DS3_COMMIT.getCreateTime()
                                                ))
                                                .setAuthor(TestData.DS3_COMMIT.getAuthor())
                                                .setMessage("Merge ds3 into r2")
                                                .build(),
                                        Common.Commit.newBuilder()
                                                .setRevision(
                                                        Common.OrderedArcRevision.newBuilder()
                                                                .setHash(TestData.DS3_COMMIT.getCommitId())
                                                                .build()
                                                )
                                                .setDate(ProtoConverter.convert(
                                                        TestData.DS3_COMMIT.getCreateTime()
                                                ))
                                                .setAuthor(TestData.DS3_COMMIT.getAuthor())
                                                .setMessage(TestData.DS3_COMMIT.getMessage())
                                                .build()
                                )
                        )
                        .setNextCommitIdPair(FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                                .setStartAt(TestData.TRUNK_COMMIT_4.getCommitId())
                                .setStopAt(StringValue.of(TestData.TRUNK_COMMIT_2.getCommitId()))
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getCommitsForFlowStart_whenRequestedOrdinaryBranch() {
        arcServiceStub.makeTrunkPointToRevision(TestData.TRUNK_COMMIT_6);

        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse response =
                grpcService.getCommitsForFlowStart(
                        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartRequest.newBuilder()
                                .setBranch("trunk")
                                .setLimit(2)
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse.newBuilder()
                        .addAllCommits(
                                List.of(
                                        Common.ArcCommit.newBuilder()
                                                .setHash(TestData.TRUNK_COMMIT_6.getCommitId())
                                                .setDate(ProtoConverter.convert(
                                                        TestData.TRUNK_COMMIT_6.getCreateTime()
                                                ))
                                                .setAuthor(TestData.TRUNK_COMMIT_6.getAuthor())
                                                .setMessage(TestData.TRUNK_COMMIT_6.getMessage())
                                                .build(),
                                        Common.ArcCommit.newBuilder()
                                                .setHash(TestData.TRUNK_COMMIT_5.getCommitId())
                                                .setDate(ProtoConverter.convert(
                                                        TestData.TRUNK_COMMIT_5.getCreateTime()
                                                ))
                                                .setAuthor(TestData.TRUNK_COMMIT_5.getAuthor())
                                                .setMessage(TestData.TRUNK_COMMIT_5.getMessage())
                                                .build()
                                )
                        )
                        .addAllCommitsWithNumber(
                                List.of(
                                        Common.Commit.newBuilder()
                                                .setRevision(ProtoMappers.toProtoOrderedArcRevision(
                                                        TestData.TRUNK_COMMIT_6.toOrderedTrunkArcRevision()
                                                ))
                                                .setDate(ProtoConverter.convert(
                                                        TestData.TRUNK_COMMIT_6.getCreateTime()
                                                ))
                                                .setAuthor(TestData.TRUNK_COMMIT_6.getAuthor())
                                                .setMessage(TestData.TRUNK_COMMIT_6.getMessage())
                                                .build(),
                                        Common.Commit.newBuilder()
                                                .setRevision(ProtoMappers.toProtoOrderedArcRevision(
                                                        TestData.TRUNK_COMMIT_5.toOrderedTrunkArcRevision()
                                                ))
                                                .setDate(ProtoConverter.convert(
                                                        TestData.TRUNK_COMMIT_5.getCreateTime()
                                                ))
                                                .setAuthor(TestData.TRUNK_COMMIT_5.getAuthor())
                                                .setMessage(TestData.TRUNK_COMMIT_5.getMessage())
                                                .build()
                                )
                        )
                        .setNextCommitIdPair(FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                                .setStartAt(TestData.TRUNK_COMMIT_4.getCommitId())
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getCommitsForFlowStart_whenRequestedRangeOfCommits() {
        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse response =
                grpcService.getCommitsForFlowStart(
                        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartRequest.newBuilder()
                                .setCommitHashIdPair(
                                        FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                                                .setStartAt(TestData.TRUNK_COMMIT_9.getCommitId())
                                                .setStopAt(StringValue.of(TestData.TRUNK_COMMIT_2.getCommitId()))
                                                .build()
                                )
                                .setLimit(2)
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse.newBuilder()
                        .addAllCommits(List.of(
                                Common.ArcCommit.newBuilder()
                                        .setHash(TestData.TRUNK_COMMIT_9.getCommitId())
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_9.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_9.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_9.getMessage())
                                        .build(),
                                Common.ArcCommit.newBuilder()
                                        .setHash(TestData.TRUNK_COMMIT_8.getCommitId())
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_8.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_8.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_8.getMessage())
                                        .build()
                        ))
                        .addAllCommitsWithNumber(List.of(
                                Common.Commit.newBuilder()
                                        .setRevision(ProtoMappers.toProtoOrderedArcRevision(
                                                TestData.TRUNK_COMMIT_9.toOrderedTrunkArcRevision()
                                        ))
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_9.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_9.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_9.getMessage())
                                        .build(),
                                Common.Commit.newBuilder()
                                        .setRevision(ProtoMappers.toProtoOrderedArcRevision(
                                                TestData.TRUNK_COMMIT_8.toOrderedTrunkArcRevision()
                                        ))
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_8.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_8.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_8.getMessage())
                                        .build()
                        ))
                        .setNextCommitIdPair(FrontendOnCommitFlowLaunchApi.CommitHashIdPair.newBuilder()
                                .setStartAt(TestData.TRUNK_COMMIT_7.getCommitId())
                                .setStopAt(StringValue.of(TestData.TRUNK_COMMIT_2.getCommitId()))
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void getCommitsForFlowStart_whenRequestedSingleCommit() {
        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse response =
                grpcService.getCommitsForFlowStart(
                        FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartRequest.newBuilder()
                                .setCommit(
                                        Common.CommitId.newBuilder()
                                                .setCommitId(TestData.TRUNK_COMMIT_9.getCommitId())
                                                .build()
                                )
                                .build()
                );
        assertThat(response).isEqualTo(
                FrontendOnCommitFlowLaunchApi.GetCommitsForFlowStartResponse.newBuilder()
                        .addAllCommits(List.of(
                                Common.ArcCommit.newBuilder()
                                        .setHash(TestData.TRUNK_COMMIT_9.getCommitId())
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_9.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_9.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_9.getMessage())
                                        .build()
                        ))
                        .addAllCommitsWithNumber(List.of(
                                Common.Commit.newBuilder()
                                        .setRevision(ProtoMappers.toProtoOrderedArcRevision(
                                                TestData.TRUNK_COMMIT_9.toOrderedTrunkArcRevision()
                                        ))
                                        .setDate(ProtoConverter.convert(
                                                TestData.TRUNK_COMMIT_9.getCreateTime()
                                        ))
                                        .setAuthor(TestData.TRUNK_COMMIT_9.getAuthor())
                                        .setMessage(TestData.TRUNK_COMMIT_9.getMessage())
                                        .build()
                        ))
                        .build()
        );
    }

    private static PullRequestDiffSet createPullRequestDiffSet(int pullRequestId) {
        PullRequest pullRequest = new PullRequest(pullRequestId, "andreevdm", "CI-4242 make ci great again", "Desc");

        PullRequestVcsInfo vcsInfo = new PullRequestVcsInfo(
                TestData.DS3_COMMIT.getRevision(),
                TestData.TRUNK_COMMIT_2.getRevision(),
                ArcBranch.trunk(),
                TestData.DS3_COMMIT.getRevision(),
                TestData.USER_BRANCH
        );

        return new PullRequestDiffSet(
                PullRequestDiffSet.Id.of(pullRequest.getId(), 21),
                pullRequest.getAuthor(),
                pullRequest.getSummary(),
                pullRequest.getDescription(),
                vcsInfo,
                null,
                List.of("CI-1", "CI-2"),
                PullRequestDiffSet.Status.NEW,
                false,
                List.of("label1", "label2"),
                null,
                null
        );
    }

    private static Launch createLaunchOnPr(LaunchId launchId, OrderedArcRevision configRevision) {
        return Launch.builder()
                .launchId(launchId)
                .flowLaunchId(FlowLaunchId.of(launchId).asString())
                .title("launch on pr 42")
                .project("ci")
                .triggeredBy("andreevdm")
                .created(Instant.parse("2020-01-02T10:00:00.000Z"))
                .type(Launch.Type.USER)
                .notifyPullRequest(true)
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(configRevision)
                                .flowId(FlowFullId.ofFlowProcessId(
                                        launchId.getProcessId()
                                ))
                                .stageGroupId("my-stages")
                                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(
                                        OrderedArcRevision.fromRevision(
                                                TestData.DS1_COMMIT.getRevision(),
                                                ArcBranch.ofPullRequest(
                                                        TestData.DIFF_SET_1.getPullRequestId()
                                                ),
                                                0L,
                                                0L
                                        )
                                )
                                .previousRevision(TestData.TRUNK_COMMIT_2.toOrderedTrunkArcRevision())
                                // prs has "-1" commit count
                                .commitCount(-1)
                                .pullRequestInfo(new LaunchPullRequestInfo(
                                        TestData.DIFF_SET_1.getPullRequestId(),
                                        TestData.DIFF_SET_1.getDiffSetId(),
                                        TestData.CI_USER,
                                        "Some test",
                                        null,
                                        ArcanumMergeRequirementId.of("CI", "pr/new: Woodcutter"),
                                        TestData.DIFF_SET_1.getVcsInfo(),
                                        TestData.DIFF_SET_1.getIssues(),
                                        TestData.DIFF_SET_1.getLabels(),
                                        null
                                ))
                                .build()
                )
                .status(LaunchState.Status.RUNNING)
                .statusText("")
                .started(Instant.parse("2020-01-03T10:00:00.000Z"))
                .finished(Instant.parse("2020-01-04T10:00:00.000Z"))
                .pinned(true)
                .tag("tag1")
                .version(Version.major("23"))
                .build();
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
