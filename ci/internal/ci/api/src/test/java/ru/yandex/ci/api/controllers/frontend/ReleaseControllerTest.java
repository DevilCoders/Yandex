package ru.yandex.ci.api.controllers.frontend;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.google.gson.JsonParser;
import io.grpc.ManagedChannel;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.GetCommitsRequest;
import ru.yandex.ci.api.internal.frontend.release.FrontendReleaseApi.UpdateAutoReleaseSettingsRequest;
import ru.yandex.ci.api.internal.frontend.release.ReleaseServiceGrpc;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigPermissions.FlowPermissions;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.assertj.core.api.InstanceOfAssertFactories.type;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.commitsResponse;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.forBranch;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequest;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithLimit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithOffset;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.freeCommitsRequestWithOffsetAndLimit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.rc;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.toCommit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.toProtoCommit;
import static ru.yandex.ci.api.controllers.frontend.ReleaseControllerTestHelper.toProtoTrunkCommit;

class ReleaseControllerTest extends ControllerTestBase<ReleaseServiceGrpc.ReleaseServiceBlockingStub> {

    private static final String BACKEND_COMPONENT = "backend";
    private static final String FRONTEND_COMPONENT = "frontend";
    private static final String NO_BRANCHES_BACKEND_COMPONENT = "no-branches-backend";
    private static final String AUTO_CREATE_BRANCH_BACKEND_COMPONENT = "auto-create-branch-backend";

    private static final Path CONFIG_PATH = Path.of("ci-tests");
    private static final String CONFIG_DIR = CONFIG_PATH.toString();
    private static final int START_REVISION = 99;

    private static final Common.Commit PROTO_R1 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_2);
    private static final Common.Commit PROTO_R5_1 = toProtoCommit(
            TestData.RELEASE_R6_1, TestData.RELEASE_BRANCH_COMMIT_6_1
    );
    private static final Common.Commit PROTO_R5_2 = toProtoCommit(
            TestData.RELEASE_R6_2, TestData.RELEASE_BRANCH_COMMIT_6_2
    );
    private static final Common.Commit PROTO_R5_3 = toProtoCommit(
            TestData.RELEASE_R6_3, TestData.RELEASE_BRANCH_COMMIT_6_3
    );
    private static final Common.Commit PROTO_R5_4 = toProtoCommit(
            TestData.RELEASE_R6_4, TestData.RELEASE_BRANCH_COMMIT_6_4
    );
    private static final Common.Commit PROTO_R3 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_3);
    private static final Common.Commit PROTO_R4 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_4);
    private static final Common.Commit PROTO_R5 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_5);
    private static final Common.Commit PROTO_R6 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_6);
    private static final Common.Commit PROTO_R7 = toProtoTrunkCommit(TestData.TRUNK_COMMIT_7);

    private static final Common.ReleaseProcessId PROTO_SIMPLE_RELEASE_ID = Common.ReleaseProcessId.newBuilder()
            .setDir(TestData.SIMPLE_RELEASE_PROCESS_ID.getDir())
            .setId(TestData.SIMPLE_RELEASE_PROCESS_ID.getSubId())
            .build();

    @Autowired
    private BranchService branchService;

    @Override
    protected ReleaseServiceGrpc.ReleaseServiceBlockingStub createStub(ManagedChannel channel) {
        return ReleaseServiceGrpc.newBlockingStub(channel);
    }

    @Test
    void getReleaseFlowsTrunkTest() {
        OrderedArcRevision revision = createAndPrepareConfiguration();

        var request = getReleaseFlowsRequest(release(FRONTEND_COMPONENT), revision);
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .setHasDisplacement(true)
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("release-frontend"))
                                .setFlowType(Common.FlowType.FT_DEFAULT)
                                .setTitle("Release frontend flow"))
                        .build());
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void getReleaseFlowsTrunkTestWithUnknownRollback() {
        OrderedArcRevision revision = createAndPrepareConfiguration();

        var unknownLaunchId = Common.ReleaseLaunchId.newBuilder()
                .setReleaseProcessId(release(FRONTEND_COMPONENT))
                .setNumber(44)
                .build();

        var request = getReleaseFlowsRequest(release(FRONTEND_COMPONENT), revision)
                .toBuilder()
                .setRollbackLaunch(unknownLaunchId)
                .build();
        assertThrows(StatusRuntimeException.class, () -> grpcService.getReleaseFlows(request));
    }

    @Test
    void getReleaseFlowsTrunkNoBranchesTest() {
        OrderedArcRevision revision = createAndPrepareConfiguration();

        var request = getReleaseFlowsRequest(release(NO_BRANCHES_BACKEND_COMPONENT), revision);
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("release-backend"))
                                .setFlowType(Common.FlowType.FT_DEFAULT)
                                .setTitle("Release backend flow"))
                        .build());
    }

    @Test
    void getReleaseFlowsTrunkAutoreleaseTest() {
        OrderedArcRevision revision = createAndPrepareConfiguration();

        var request = getReleaseFlowsRequest(release(AUTO_CREATE_BRANCH_BACKEND_COMPONENT), revision);
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("release-backend"))
                                .setFlowType(Common.FlowType.FT_DEFAULT)
                                .setTitle("Release backend flow"))
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("hotfix-backend"))
                                .setTitle("Hotfix backend flow")
                                .setFlowType(Common.FlowType.FT_HOTFIX)
                                .setDescription("Run hotfix"))
                        .build()
        );
    }

    @Test
    void getReleaseFlowsBackendBranchTest() {
        var trunkRevision = createAndPrepareConfiguration();
        var request = getReleaseFlowsRequest(release(BACKEND_COMPONENT), trunkRevision);

        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("release-backend"))
                                .setFlowType(Common.FlowType.FT_DEFAULT)
                                .setTitle("Release backend flow"))
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("hotfix-backend"))
                                .setFlowType(Common.FlowType.FT_HOTFIX)
                                .setTitle("Hotfix backend flow")
                                .setDescription("Run hotfix"))
                        .build()
        );
    }

    @Test
    void getReleaseFlowsFrontendBranchTest() {

        var trunkRevision = createAndPrepareConfiguration();
        var request = getReleaseFlowsRequest(release(FRONTEND_COMPONENT), trunkRevision);

        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .setHasDisplacement(true)
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("release-frontend"))
                                .setFlowType(Common.FlowType.FT_DEFAULT)
                                .setTitle("Release frontend flow"))
                        .build()
        );
    }

    @Test
    void launchReleaseWithFlowVars() {
        var revision = createAndPrepareConfiguration();
        var processId = release("with-flow-vars");

        Common.FlowVars flowVars = Common.FlowVars.newBuilder()
                .setJson("""
                        {
                            "title": "My Super-release",
                            "iterations": 7,
                            "stages": {
                                "stable": true,
                                "prestable": false,
                                "testing": true
                            }
                        }
                        """)
                .build();

        var releaseFlows = grpcService.getReleaseFlows(getReleaseFlowsRequest(processId, revision));

        assertThat(releaseFlows.getFlowsList())
                .singleElement()
                .isEqualTo(
                        TestUtils.parseProtoText("release/getReleaseFlowsResponse.pb", Common.FlowDescription.class)
                );

        FrontendReleaseApi.StartReleaseResponse startRelease = grpcService.startRelease(
                startReleaseRequest(processId, START_REVISION)
                        .toBuilder()
                        .setFlowVars(flowVars)
                        .build()
        );

        assertThat(startRelease.hasReleaseLaunch()).isTrue();

        assertThat(startRelease.getReleaseLaunch().getDisplacementChangesList())
                .isEmpty();

        var launch = db.currentOrReadOnly(() ->
                db.launches().findOptional(ProtoMappers.toLaunchId(startRelease.getReleaseLaunch().getId()))
                        .orElseThrow()
        );

        var actualFlowVars = launch.getFlowInfo().getFlowVars();
        assertThat(actualFlowVars).isNotNull();
        assertThat(actualFlowVars.getData())
                .isEqualTo(JsonParser.parseString("""
                        {
                            "declared-in-release-flow-vars": "in release!",
                            "title": "My Super-release",
                            "iterations": 7,
                            "stages": {
                                "stable": true,
                                "prestable": false,
                                "testing": true
                            }
                        }"""));
    }

    @Test
    void withoutRequiredFlowVar() {
        createAndPrepareConfiguration();
        var processId = release("with-flow-vars");

        Common.FlowVars flowVars = Common.FlowVars.newBuilder()
                .setJson("""
                        {
                            "title": "My Super-release",
                            "stages": {
                                "stable": true,
                                "prestable": false,
                                "testing": true
                            }
                        }
                        """)
                .build();

        assertThatThrownBy(() -> grpcService.startRelease(
                startReleaseRequest(processId, START_REVISION)
                        .toBuilder()
                        .setFlowVars(flowVars)
                        .build()
        ))
                .hasMessageContaining(" are not valid according to schema")
                .hasMessageContaining("object has missing required properties ([\\\"iterations\\\"])");
    }

    @Test
    void launchReleaseTest() {
        var revision = createAndPrepareConfiguration();

        var startReleaseRequest = startReleaseRequest(release(FRONTEND_COMPONENT), START_REVISION).toBuilder()
                .setPreventDisplacement(true)
                .build();
        FrontendReleaseApi.StartReleaseResponse startRelease = grpcService.startRelease(startReleaseRequest);
        assertThat(startRelease.hasReleaseLaunch()).isTrue();
        prepareLaunch(startRelease);

        FrontendReleaseApi.GetReleasesResponse releasesResponse =
                grpcService.getReleases(
                        FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(release(FRONTEND_COMPONENT))
                                .build()
                );

        assertThat(releasesResponse.getReleasesList()).hasSize(1);
        Common.ReleaseLaunch launch = releasesResponse.getReleases(0);

        assertThat(launch.getId().getReleaseProcessId()).isEqualTo(release(FRONTEND_COMPONENT));
        assertThat(launch.getCommitCount()).isEqualTo(1);
        assertThat(launch.getRevision()).isEqualTo(ProtoMappers.toProtoOrderedArcRevision(revision(START_REVISION)));

        assertThat(launch.getDisplacementChangesList())
                .isEqualTo(List.of(Common.DisplacementChange.newBuilder()
                        .setState(Common.DisplacementState.DS_DENY)
                        .setChangedBy("user42")
                        .setChangedAt(ProtoMappers.toProtoTimestamp(NOW))
                        .build()));

        assertThat(launch.getStagesState())
                .isEqualTo(Common.StagesState.newBuilder()
                        .build());


        var getReleaseFlowsRequest = getReleaseFlowsRequest(release(FRONTEND_COMPONENT), revision)
                .toBuilder()
                .setRollbackLaunch(startRelease.getReleaseLaunch().getId())
                .build();
        var response = grpcService.getReleaseFlows(getReleaseFlowsRequest);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .setHasDisplacement(true)
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("rollback_frontend_release-frontend"))
                                .setFlowType(Common.FlowType.FT_ROLLBACK)
                                .setTitle("Release frontend flow"))
                        .build());
    }

    @Test
    void launchBranchReleaseTestWithCustomHotfixFlow() {
        var component = BACKEND_COMPONENT;

        var processId = ciProcess(component);
        var releaseId = release(component);

        var flowId = ProtoMappers.toProtoFlowProcessId(CiProcessId.ofFlow(processId.getPath(), "hotfix-backend"));

        var trunkRevision = createAndPrepareConfiguration();

        var startRelease = launchBackendComponentRelease(flowId);
        prepareLaunch(startRelease);
        assertThat(startRelease.hasReleaseLaunch()).isTrue();

        FrontendReleaseApi.GetReleasesResponse releasesResponse =
                grpcService.getReleases(
                        FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(releaseId)
                                .build()
                );

        assertThat(releasesResponse.getReleasesList()).hasSize(1);
        var launch = releasesResponse.getReleases(0);

        assertThat(launch.getId().getReleaseProcessId())
                .isEqualTo(release(component));

        assertThat(launch.getCommitCount())
                .isEqualTo(1);

        assertThat(launch.getRevision())
                .isEqualTo(ProtoMappers.toProtoOrderedArcRevision(trunkRevision));

        assertThat(launch.getFlowDescription())
                .isEqualTo(Common.FlowDescription.newBuilder()
                        .setFlowProcessId(flowId)
                        .setTitle("Hotfix backend flow")
                        .setDescription("Run hotfix")
                        .setFlowType(Common.FlowType.FT_HOTFIX)
                        .addRollbackFlows(flowId.toBuilder().setId("rollback-backend").build())
                        .addRollbackFlows(flowId.toBuilder().setId("rollback_backend_hotfix-backend").build())
                        .build());

        var request = getReleaseFlowsRequest(release(component), trunkRevision)
                .toBuilder()
                .setRollbackLaunch(startRelease.getReleaseLaunch().getId())
                .build();
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("rollback-backend"))
                                .setTitle("Rollback backend flow")
                                .setFlowType(Common.FlowType.FT_ROLLBACK)
                                .setDescription("Run rollback"))
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("rollback_backend_hotfix-backend"))
                                .setTitle("Hotfix backend flow")
                                .setFlowType(Common.FlowType.FT_ROLLBACK)
                                .setDescription("Run hotfix"))
                        .build());
    }

    @Test
    void launchBranchReleaseTestWithCustomRollbackFlow() {
        var firstRelease = launchBackendComponentRelease(null);
        prepareLaunch(firstRelease);

        var component = BACKEND_COMPONENT;
        var trunkRevision = createAndPrepareConfiguration();

        var request = getReleaseFlowsRequest(release(component), trunkRevision)
                .toBuilder()
                .setRollbackLaunch(firstRelease.getReleaseLaunch().getId())
                .build();

        // rollback-backend - только для hotfix-backend
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("rollback_backend_release-backend"))
                                .setTitle("Release backend flow")
                                .setFlowType(Common.FlowType.FT_ROLLBACK))
                        .build());


        var processId = ciProcess(component);
        var releaseId = release(component);

        var flowId = ProtoMappers.toProtoFlowProcessId(CiProcessId.ofFlow(processId.getPath(), "rollback-backend"));
        var branch = createBranch(processId, trunkRevision).getArcBranch();

        var startReleaseRequest = FrontendReleaseApi.StartReleaseRequest.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(processId))
                .setCommit(ProtoMappers.toCommitId(trunkRevision))
                .setBranch(branch.asString())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(trunkRevision))
                .setFlowProcessId(flowId)
                .setFlowType(Common.FlowType.FT_ROLLBACK)
                .setRollbackLaunch(firstRelease.getReleaseLaunch().getId())
                .build();

        var startRelease = grpcService.startRelease(startReleaseRequest);
        assertThat(startRelease.hasReleaseLaunch()).isTrue();

        FrontendReleaseApi.GetReleasesResponse releasesResponse =
                grpcService.getReleases(
                        FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(releaseId)
                                .build()
                );

        assertThat(releasesResponse.getReleasesList()).hasSize(2);
        var launch = releasesResponse.getReleases(0);

        assertThat(launch.getId().getReleaseProcessId())
                .isEqualTo(release(component));

        assertThat(launch.getCommitCount())
                .isEqualTo(0);

        assertThat(launch.getRevision())
                .isEqualTo(ProtoMappers.toProtoOrderedArcRevision(trunkRevision));

        assertThat(launch.getFlowDescription())
                .isEqualTo(Common.FlowDescription.newBuilder()
                        .setFlowProcessId(flowId)
                        .setTitle("Rollback backend flow")
                        .setDescription("Run rollback")
                        .setFlowType(Common.FlowType.FT_ROLLBACK)
                        .build());

        assertThat(launch.getStagesState())
                .isEqualTo(Common.StagesState.newBuilder()
                        .build());
    }

    @Test
    void launchRollbackBranchReleaseTest() {
        var firstRelease = launchBackendComponentRelease(null);
        prepareLaunch(firstRelease);

        var component = BACKEND_COMPONENT;
        var trunkRevision = createAndPrepareConfiguration();

        var request = getReleaseFlowsRequest(release(component), trunkRevision)
                .toBuilder()
                .setRollbackLaunch(firstRelease.getReleaseLaunch().getId())
                .build();

        // rollback-backend - только для hotfix-backend
        var response = grpcService.getReleaseFlows(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.GetReleaseFlowsResponse.newBuilder()
                        .addFlows(Common.FlowDescription.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir(CONFIG_DIR)
                                        .setId("rollback_backend_release-backend"))
                                .setTitle("Release backend flow")
                                .setFlowType(Common.FlowType.FT_ROLLBACK))
                        .build());


        var processId = ciProcess(component);
        var releaseId = release(component);

        var flowId = ProtoMappers.toProtoFlowProcessId(CiProcessId.ofFlow(processId.getPath(), "rollback-backend"));
        var startRollbackReleaseRequest = FrontendReleaseApi.StartRollbackReleaseRequest.newBuilder()
                .setRollbackLaunch(firstRelease.getReleaseLaunch().getId())
                .setFlowProcessId(flowId)
                .setRollbackReason("Because we can")
                .setCancelOthers(true)
                .setDisableAutorelease(true)
                .build();

        var startRelease = grpcService.startRollbackRelease(startRollbackReleaseRequest);
        assertThat(startRelease.hasReleaseLaunch()).isTrue();

        FrontendReleaseApi.GetReleasesResponse releasesResponse =
                grpcService.getReleases(
                        FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(releaseId)
                                .build()
                );

        assertThat(releasesResponse.getReleasesList()).hasSize(2);
        var launch = releasesResponse.getReleases(0);

        assertThat(launch.getId().getReleaseProcessId())
                .isEqualTo(release(component));

        assertThat(launch.getCommitCount())
                .isEqualTo(0);

        assertThat(launch.getRevision())
                .isEqualTo(ProtoMappers.toProtoOrderedArcRevision(trunkRevision));

        assertThat(launch.getFlowDescription())
                .isEqualTo(Common.FlowDescription.newBuilder()
                        .setFlowProcessId(flowId)
                        .setTitle("Rollback backend flow")
                        .setDescription("Run rollback")
                        .setFlowType(Common.FlowType.FT_ROLLBACK)
                        .build());

        assertThat(launch.getLaunchReason())
                .isEqualTo("Because we can");
    }

    @Test
    void getReleaseTest() {
        createAndPrepareConfiguration();

        var component = FRONTEND_COMPONENT;

        Common.ReleaseLaunchId releaseLaunchId = grpcService.startRelease(
                startReleaseRequest(release(component), START_REVISION)
        ).getReleaseLaunch().getId();

        var flowId = FlowFullId.of(ciProcess(component).getPath(), "release-frontend");
        var protoFlowId = ProtoMappers.toProtoFlowProcessId(flowId);
        var flowDescription = Common.FlowDescription.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(flowId))
                .setTitle("Release frontend flow")
                .setFlowType(Common.FlowType.FT_DEFAULT)
                .addRollbackFlows(protoFlowId.toBuilder().setId("rollback_frontend_release-frontend").build())
                .build();

        var response = FrontendReleaseApi.GetReleaseResponse.newBuilder()
                .setRelease(
                        Common.ReleaseLaunch.newBuilder()
                                .setId(releaseLaunchId)
                                .setTitle("Release frontend component #" + releaseLaunchId.getNumber())
                                .setTriggeredBy("user42")
                                .setRevision(ProtoMappers.toProtoOrderedArcRevision(revision(START_REVISION)))
                                .setCommitCount(1)
                                .setCreated(ProtoMappers.toProtoTimestamp(NOW))
                                .setCancelable(true)
                                .setFlowDescription(flowDescription)
                                .setStatus(Common.LaunchStatus.STARTING)
                                .setHasDisplacement(true)
                                .setVersion(Common.Version.newBuilder()
                                        .setFull("1")
                                        .setMajor("1")
                                        .build())
                                .build()
                )
                .setProject(ProjectControllerTest.project(Abc.CI))
                .setReleaseState(
                        Common.ReleaseState.newBuilder()
                                .setId(releaseLaunchId.getReleaseProcessId())
                                .setTitle("Release frontend component")
                                .setDescription("Description for frontend release")
                                .setAuto(Common.AutoReleaseState.newBuilder()
                                        .setEnabled(false)
                                        .setEditable(false)
                                        .build())
                                .setReleaseFromTrunkAllowed(true)
                                .setReleaseBranchesEnabled(true)
                                .setDefaultConfigFromBranch(true)
                                .addBranches(Common.Branch.newBuilder()
                                        .setName(ArcBranch.trunk().getBranch())
                                        .build())
                                .build()
                )
                .build();

        var requestByNumber = FrontendReleaseApi.GetReleaseRequest.newBuilder()
                .setId(releaseLaunchId)
                .build();
        assertThat(grpcService.getRelease(requestByNumber))
                .isEqualTo(response);

        var launchIdNoNumber = releaseLaunchId.toBuilder()
                .clearNumber()
                .build();
        var requestByVersionMajorMinor = FrontendReleaseApi.GetReleaseRequest.newBuilder()
                .setId(launchIdNoNumber)
                .setVersion(Common.Version.newBuilder()
                        .setMajor("1"))
                .build();
        assertThat(grpcService.getRelease(requestByVersionMajorMinor))
                .isEqualTo(response);


        var requestByVersionFull = FrontendReleaseApi.GetReleaseRequest.newBuilder()
                .setId(launchIdNoNumber)
                .setVersion(Common.Version.newBuilder()
                        .setFull("1"))
                .build();
        assertThat(grpcService.getRelease(requestByVersionFull))
                .isEqualTo(response);


        var requestByPriorityNumber = FrontendReleaseApi.GetReleaseRequest.newBuilder()
                .setId(releaseLaunchId)
                .setVersion(Common.Version.newBuilder()
                        .setMajor("2")
                        .setMinor("3")
                        .setFull("4.5"))
                .build();
        assertThat(grpcService.getRelease(requestByPriorityNumber))
                .isEqualTo(response);

        var requestByPriorityMajor = FrontendReleaseApi.GetReleaseRequest.newBuilder()
                .setId(launchIdNoNumber)
                .setVersion(Common.Version.newBuilder()
                        .setMajor("1")
                        .setFull("4.5"))
                .build();
        assertThat(grpcService.getRelease(requestByPriorityMajor))
                .isEqualTo(response);

        //

        var afterDeny = grpcService.changeDisplacementState(FrontendReleaseApi.DisplacementChangeRequest.newBuilder()
                .setId(releaseLaunchId)
                .setState(Common.DisplacementState.DS_DENY)
                .build());

        var releaseAfterDeny = response.getRelease().toBuilder()
                .addDisplacementChanges(Common.DisplacementChange.newBuilder()
                        .setState(Common.DisplacementState.DS_DENY)
                        .setChangedBy("user42")
                        .setChangedAt(ProtoMappers.toProtoTimestamp(NOW))
                        .build())
                .build();

        assertThat(afterDeny)
                .extracting(FrontendReleaseApi.DisplacementChangeResponse::getRelease)
                .isEqualTo(releaseAfterDeny);


        var afterAllow = grpcService.changeDisplacementState(FrontendReleaseApi.DisplacementChangeRequest.newBuilder()
                .setId(releaseLaunchId)
                .setState(Common.DisplacementState.DS_ALLOW)
                .build());

        var releaseAfterAllow = response.getRelease().toBuilder()
                .addDisplacementChanges(Common.DisplacementChange.newBuilder()
                        .setState(Common.DisplacementState.DS_DENY)
                        .setChangedBy("user42")
                        .setChangedAt(ProtoMappers.toProtoTimestamp(NOW))
                        .build())
                .addDisplacementChanges(Common.DisplacementChange.newBuilder()
                        .setState(Common.DisplacementState.DS_ALLOW)
                        .setChangedBy("user42")
                        .setChangedAt(ProtoMappers.toProtoTimestamp(NOW))
                        .build())
                .build();

        assertThat(afterAllow)
                .extracting(FrontendReleaseApi.DisplacementChangeResponse::getRelease)
                .isEqualTo(releaseAfterAllow);
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void getReleasesWithoutCancelled() {
        createAndPrepareConfiguration();
        Common.ReleaseProcessId processId = release(BACKEND_COMPONENT);

        grpcService.startRelease(startReleaseRequest(processId, START_REVISION));

        doCommitAndDiscover(START_REVISION + 1);
        grpcService.startRelease(startReleaseRequest(processId, START_REVISION + 1));

        doCommitAndDiscover(START_REVISION + 2);
        grpcService.startRelease(startReleaseRequest(processId, START_REVISION + 2));

        List<Common.ReleaseLaunch> releases =
                grpcService.getReleases(FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(processId)
                                .build())
                        .getReleasesList();

        assertThat(releases).hasSize(3);

        Common.ReleaseLaunchId cancelledId = releases.get(1).getId();
        FrontendReleaseApi.CancelReleaseResponse cancelingRelease = grpcService.cancelRelease(
                FrontendReleaseApi.CancelReleaseRequest.newBuilder().setId(cancelledId).build()
        );

        assertThat(cancelingRelease.getRelease().getStatus())
                .isEqualTo(Common.LaunchStatus.CANCELLING);

        assertThat(cancelingRelease.getRelease().getId())
                .isEqualTo(cancelledId);

        db.currentOrTx(() -> {
            Launch launch = db.launches().get(ProtoMappers.toLaunchId(cancelledId));
            db.launches().save(
                    launch.toBuilder()
                            .status(LaunchState.Status.CANCELED)
                            .statusText("")
                            .build()
            );
        });

        List<Common.ReleaseLaunch> releasesAfterCancel =
                grpcService.getReleases(FrontendReleaseApi.GetReleasesRequest.newBuilder()
                                .setReleaseProcessId(processId)
                                .setDontReturnCancelled(true)
                                .build())
                        .getReleasesList();

        assertThat(releasesAfterCancel).hasSize(2)
                .extracting(Common.ReleaseLaunch::getId)
                .containsExactly(releases.get(0).getId(), releases.get(2).getId());

    }


    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void cancelPreviousRelease() {
        createAndPrepareConfiguration();

        grpcService.startRelease(
                startReleaseRequest(release(BACKEND_COMPONENT), START_REVISION)
        );

        doCommitAndDiscover(START_REVISION + 1);

        grpcService.startRelease(
                startReleaseRequest(release(BACKEND_COMPONENT), START_REVISION + 1)
        );

        FrontendReleaseApi.GetReleasesResponse releasesResponse =
                grpcService.getReleases(FrontendReleaseApi.GetReleasesRequest.newBuilder()
                        .setReleaseProcessId(release(BACKEND_COMPONENT))
                        .build());

        assertThat(releasesResponse.getReleasesList()).hasSize(2);
        Common.ReleaseLaunch latestRelease = releasesResponse.getReleases(0);
        Common.ReleaseLaunch previousRelease = releasesResponse.getReleases(1);

        assertThat(latestRelease.getRevision().getNumber()).isEqualTo(START_REVISION + 1);
        assertThat(previousRelease.getRevision().getNumber()).isEqualTo(START_REVISION);
        assertThat(latestRelease.getCommitCount()).isEqualTo(1);
        assertThat(latestRelease.getPreviousRevision()).isEqualTo(previousRelease.getRevision());
        assertThat(previousRelease.hasPreviousRevision()).isFalse();

        grpcService.cancelRelease(
                FrontendReleaseApi.CancelReleaseRequest.newBuilder()
                        .setId(previousRelease.getId())
                        .setReason("Reason")
                        .build()
        );

        releasesResponse =
                grpcService.getReleases(FrontendReleaseApi.GetReleasesRequest.newBuilder()
                        .setReleaseProcessId(release(BACKEND_COMPONENT))
                        .build());

        assertThat(releasesResponse.getReleasesList()).hasSize(2);
        latestRelease = releasesResponse.getReleases(0);
        previousRelease = releasesResponse.getReleases(1);
        // update should be performed after actual cancelling
        assertThat(latestRelease.getRevision().getNumber()).isEqualTo(START_REVISION + 1);
        assertThat(latestRelease.getCommitCount()).isEqualTo(1);
        assertThat(latestRelease.getCancelledReleasesCount()).isEqualTo(0);

        assertThat(previousRelease.hasPreviousRevision()).isFalse();
        assertThat(previousRelease.getCancelledBy()).isEqualTo("user42");
        assertThat(previousRelease.getCancelledReason()).isEqualTo("Reason");
    }

    @Test
    void getUnattachedCommitsWithoutRelease() {
        discoveryToR6();

        assertThat(
                grpcService.getCommits(
                        forProcess(PROTO_SIMPLE_RELEASE_ID)
                )
        ).isEqualTo(
                commitsResponse(5, false, PROTO_R6, PROTO_R5, PROTO_R4, PROTO_R3, PROTO_R1)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, true, PROTO_R6, PROTO_R5)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setOffsetCommitNumber(5)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, false, PROTO_R4, PROTO_R3, PROTO_R1)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setOffsetCommitNumber(5)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, true, PROTO_R4, PROTO_R3)
        );
    }

    @Test
    void getUnattachedCommitsWithoutReleaseInBranch() {
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
                        rc(PROTO_R6).addBranches(branch), rc(PROTO_R5), rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R1)
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
                        rc(PROTO_R6).addBranches(branch), rc(PROTO_R5), rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R1)
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequest(processId, ArcBranch.trunk().asString()))
        ).isEqualTo(
                commitsResponse(1, false,
                        PROTO_R7
                )
        );

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithOffsetAndLimit(processId, branch, 2, 2))
        ).isEqualTo(
                commitsResponse(9, true,
                        rc(PROTO_R5_1), rc(PROTO_R6).addBranches(branch)
                )
        );
    }

    @Test
    void getUnattachedCommitsWithRelease() {
        discoveryToR7();

        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        launchService.startRelease(
                TestData.SIMPLE_RELEASE_PROCESS_ID, TestData.TRUNK_R3.toRevision(), ArcBranch.trunk(),
                "andreevdm", null, false, false, null, true, null, null, null);


        assertThat(
                grpcService.getCommits(
                        forProcess(PROTO_SIMPLE_RELEASE_ID)
                )
        ).isEqualTo(
                commitsResponse(4, false, PROTO_R7, PROTO_R6, PROTO_R5, PROTO_R4)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(4, true, PROTO_R7, PROTO_R6)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setOffsetCommitNumber(7)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(4, false, PROTO_R6, PROTO_R5, PROTO_R4)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                                .setOffsetCommitNumber(7)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(4, true, PROTO_R6, PROTO_R5)
        );
    }

    @Test
    void getUnattachedCommitsInBranchWithReleaseAtTrunk() {
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
    void getUnattachedCommitsWithBranch() {
        var releaseId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;
        var protoReleaseId = ProtoMappers.toProtoReleaseProcessId(TestData.WITH_BRANCHES_RELEASE_PROCESS_ID);

        discoveryToR7();

        assertThat(grpcService.getCommits(forProcess(protoReleaseId)))
                .isEqualTo(commitsResponse(6, false, PROTO_R7, PROTO_R6, PROTO_R5, PROTO_R4, PROTO_R3, PROTO_R1));

        createBranch(releaseId, TestData.TRUNK_R4);

        assertThat(grpcService.getCommits(forProcess(protoReleaseId)))
                .isEqualTo(commitsResponse(3, false, PROTO_R7, PROTO_R6, PROTO_R5));
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
                        rc(PROTO_R4).addBranches(branchR3Name),
                        rc(PROTO_R3),
                        rc(PROTO_R1))
                );

        Branch branchR5 = db.currentOrTx(() -> createBranch(releaseId, TestData.TRUNK_R6));
        String branchR5Name = branchR5.getArcBranch().asString();

        assertThat(grpcService.getCommits(forBranch(branchR5)))
                .describedAs("Should contain commits up to next timeline item")
                .isEqualTo(commitsResponse(2, false,
                        rc(PROTO_R6).addBranches(branchR5Name),
                        rc(PROTO_R5))
                );

        OrderedArcRevision revisionR5n1 = revision(1, branchR5.getArcBranch());
        ArcCommit commitR5n1 = toCommit(revisionR5n1);
        arcServiceStub.addCommit(commitR5n1, TestData.TRUNK_COMMIT_6,
                Map.of(releaseId.getPath().resolve("some.txt"), ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(revisionR5n1.getBranch(), revisionR5n1.toRevision(), false);

        assertThat(
                grpcService.getCommits(freeCommitsRequestWithLimit(releaseId, branchR5.getArcBranch().asString(), 2))
        )
                .describedAs("Free commits should return commits from trunk as well")
                .isEqualTo(commitsResponse(3, true,
                                rc(toProtoCommit(revisionR5n1, commitR5n1)),
                                rc(PROTO_R6).addBranches(branchR5Name)
                        )
                );

        assertThat(grpcService.getCommits(forBranch(branchR5)))
                .describedAs("Should contain only commits in trunk")
                .isEqualTo(commitsResponse(2, false, rc(PROTO_R6).addBranches(branchR5Name), rc(PROTO_R5)));
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
                Map.of(releaseId.getPath().resolve("some.txt"), ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(revisionR5n1.getBranch(), revisionR5n1.toRevision(), false);

        assertThat(
                grpcService.getCommits(freeCommitsRequest(releaseId, branchR5.getArcBranch().asString()))
        )
                .describedAs("Should contain commits in branch and in trunk as well")
                .isEqualTo(commitsResponse(6, false,
                        rc(toProtoCommit(revisionR5n1, commitR5n1)),
                        rc(PROTO_R6).addBranches(branchR5Name),
                        rc(PROTO_R5), rc(PROTO_R4), rc(PROTO_R3), rc(PROTO_R1)
                ));

        assertThat(grpcService.getCommits(
                freeCommitsRequestWithOffset(
                        releaseId, branchR5.getArcBranch().asString(), "trunk", TestData.TRUNK_R4.getNumber()
                )
        ))
                .describedAs("Should contain commits not including supplied in offset")
                .isEqualTo(commitsResponse(6, false,
                        rc(PROTO_R3), rc(PROTO_R1)
                ));
    }

    @Test
    void getCommitsInRelease() {
        discoveryToR7();

        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        Launch launch = launchService.startRelease(
                TestData.SIMPLE_RELEASE_PROCESS_ID, TestData.TRUNK_R6.toRevision(), ArcBranch.trunk(),
                "andreevdm", null, false, false, null, true, null, null, null);
        Common.ReleaseLaunchId releaseLaunchId = Common.ReleaseLaunchId.newBuilder()
                .setReleaseProcessId(PROTO_SIMPLE_RELEASE_ID)
                .setNumber(launch.getLaunchId().getNumber())
                .build();

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseLaunchId(releaseLaunchId)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, false, PROTO_R6, PROTO_R5, PROTO_R4, PROTO_R3, PROTO_R1)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseLaunchId(releaseLaunchId)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, true, PROTO_R6, PROTO_R5)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseLaunchId(releaseLaunchId)
                                .setOffsetCommitNumber(5)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, false, PROTO_R4, PROTO_R3, PROTO_R1)
        );

        assertThat(
                grpcService.getCommits(
                        GetCommitsRequest.newBuilder()
                                .setReleaseLaunchId(releaseLaunchId)
                                .setOffsetCommitNumber(5)
                                .setLimit(2)
                                .build()
                )
        ).isEqualTo(
                commitsResponse(5, true, PROTO_R4, PROTO_R3)
        );
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void updateAutoReleaseSettings_throwsException() {
        UpdateAutoReleaseSettingsRequest request =
                UpdateAutoReleaseSettingsRequest.newBuilder()
                        .setReleaseProcessId(
                                release(TestData.RELEASE_PROCESS_ID.getSubId())
                        )
                        .setEnabled(false)
                        .setMessage("Disabled auto-release until I'll fix bugs")
                        .build();
        assertThrows(
                StatusRuntimeException.class,
                () -> grpcService.updateAutoReleaseSettings(request),
                "can not update auto release settings, cause auto release is disabled in config"
        );
    }

    @Test
    void updateAutoReleaseSettings() {
        discoveryToR2();
        delegateToken(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getPath());
        UpdateAutoReleaseSettingsRequest request =
                UpdateAutoReleaseSettingsRequest.newBuilder()
                        .setReleaseProcessId(
                                Common.ReleaseProcessId.newBuilder()
                                        .setDir(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getDir())
                                        .setId(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getSubId())
                                        .build()
                        )
                        .setEnabled(false)
                        .setMessage("Disabled auto-release until I'll fix bugs")
                        .build();
        var response = grpcService.updateAutoReleaseSettings(request);
        assertThat(response).isEqualTo(
                FrontendReleaseApi.UpdateAutoReleaseSettingsResponse.newBuilder()
                        .setAuto(Common.AutoReleaseState.newBuilder()
                                .setEnabled(false)
                                .setEditable(true)
                                .setLastManualAction(Common.AutoReleaseState.LastManualAction.newBuilder()
                                        .setEnabled(false)
                                        .setLogin("user42")
                                        .setMessage("Disabled auto-release until I'll fix bugs")
                                        .setDate(response.getAuto().getLastManualAction().getDate())
                                )
                                .build()
                        )
                        .build()
        );
    }

    @Test
    void checkPermissionsState() {
        var revision = createAndPrepareConfiguration();

        var path = CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME);
        db.currentOrReadOnly(() -> db.configStates().get(path)); // No permissions but it's OK

        var basePermissions = Permissions.builder()
                .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("ci", "test"))
                .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("ci", "test"))
                .build();

        var simpleActionPermissions = Permissions.builder()
                .add(PermissionScope.START_FLOW,
                        PermissionRule.ofScopes("abc"),
                        PermissionRule.ofScopes("ci", "administration")
                )
                .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("ci", "test"))
                .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci", "admin"))
                .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("ci", "test"))
                .build();

        var frontendPermissions = Permissions.builder()
                .add(PermissionScope.START_FLOW,
                        PermissionRule.ofScopes("some"),
                        PermissionRule.ofScopes("service"),
                        PermissionRule.ofScopes("ci", "administration")
                )
                .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("ci", "test"))
                .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci", "admin"))
                .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("ci", "test"))
                .build();

        var withFlowVarsPermissions = Permissions.builder()
                .add(PermissionScope.START_FLOW,
                        PermissionRule.ofScopes("cidemo"),
                        PermissionRule.ofScopes("ci")
                )
                .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("ci", "test"))
                .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("ci", "test"))
                .build();

        var config = db.currentOrReadOnly(() -> db.configHistory().getById(path, revision));
        assertThat(config.getPermissions())
                .isNotNull();
        assertThat(config.getPermissions())
                .isEqualTo(ConfigPermissions.builder()
                        .project("ci")
                        .action("simple-action", simpleActionPermissions)
                        .release("backend", basePermissions)
                        .release("frontend", frontendPermissions)
                        .release("no-branches-backend", basePermissions)
                        .release("auto-create-branch-backend", basePermissions)
                        .release("with-flow-vars", withFlowVarsPermissions)
                        .flow("release-frontend", FlowPermissions.builder()
                                .jobApprover("single", List.of(PermissionRule.ofScopes("abc")))
                                .build())
                        .flow("release-backend", FlowPermissions.builder()
                                .jobApprover("single", List.of(PermissionRule.ofScopes("abc1", "cde1")))
                                .jobApprover("last", List.of(PermissionRule.ofScopes("abc2", "cde2")))
                                .build())
                        .flow("rollback_backend_release-backend", FlowPermissions.builder()
                                .jobApprover("single", List.of(PermissionRule.ofScopes("abc1", "cde1")))
                                .jobApprover("last", List.of(PermissionRule.ofScopes("abc2", "cde2")))
                                .build())
                        .flow("rollback_frontend_release-frontend", FlowPermissions.builder()
                                .jobApprover("single", List.of(PermissionRule.ofScopes("abc")))
                                .build())
                        .build());
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Nested
    class CheckAccess {

        @Test
        void startRelease() {
            createAndPrepareConfiguration();

            abcServiceStub.addService(Abc.CI, "another-member");

            assertThatThrownBy(() ->
                    grpcService.startRelease(
                            startReleaseRequest(release(BACKEND_COMPONENT), START_REVISION)
                    ))
                    .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [START_FLOW], " +
                            "not in project [ci]")
                    .asInstanceOf(type(StatusRuntimeException.class))
                    .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
        }

        @Test
        void startReleaseWithCustomPermissions() {
            createAndPrepareConfiguration();

            abcServiceStub.addService(Abc.CI, "another-member");

            assertThatThrownBy(() ->
                    grpcService.startRelease(
                            startReleaseRequest(release(FRONTEND_COMPONENT), START_REVISION)
                    ))
                    .hasMessageContaining("PERMISSION_DENIED: User [user42] has no access to scope [START_FLOW] " +
                            "within rules [" +
                            "[ABC Service = some], " +
                            "[ABC Service = service], " +
                            "[ABC Service = ci, scopes = [administration]]" +
                            "]")
                    .asInstanceOf(type(StatusRuntimeException.class))
                    .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
        }

        @Test
        void cancelRelease() {
            createAndPrepareConfiguration();

            FrontendReleaseApi.StartReleaseResponse response = grpcService.startRelease(
                    startReleaseRequest(release(BACKEND_COMPONENT), START_REVISION)
            );

            abcServiceStub.addService(Abc.CI, "another-member");

            assertThatThrownBy(() ->
                    grpcService.cancelRelease(FrontendReleaseApi.CancelReleaseRequest.newBuilder()
                            .setId(response.getReleaseLaunch().getId())
                            .build())
            )
                    .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [CANCEL_FLOW]," +
                            " not in project [ci]")
                    .asInstanceOf(type(StatusRuntimeException.class))
                    .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
        }

        @Test
        void switchReleaseState() {
            discoveryToR2();
            delegateToken(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getPath());

            abcServiceStub.addService(Abc.CI, "another-member");

            assertThatThrownBy(() ->
                    grpcService.updateAutoReleaseSettings(UpdateAutoReleaseSettingsRequest.newBuilder()
                            .setReleaseProcessId(
                                    Common.ReleaseProcessId.newBuilder()
                                            .setDir(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getDir())
                                            .setId(TestData.AUTO_RELEASE_SIMPLE_PROCESS_ID.getSubId())
                                            .build()
                            )
                            .setEnabled(false)
                            .build())
            )
                    .hasMessage("PERMISSION_DENIED: User [user42] has no access to scope [TOGGLE_AUTORUN], " +
                            "not in project [ci]")
                    .asInstanceOf(type(StatusRuntimeException.class))
                    .returns(Status.Code.PERMISSION_DENIED, s -> s.getStatus().getCode());
        }
    }

    private void doCommitAndDiscover(int commitNumber) {
        OrderedArcRevision revision = revision(commitNumber);
        doCommit(revision, Map.of(CONFIG_PATH.resolve("SomeCode.java"), ChangeType.Modify));
        discoveryServicePostCommits.processPostCommit(revision.getBranch(), revision.toRevision(), false);
    }

    private FrontendReleaseApi.StartReleaseResponse launchBackendComponentRelease(
            @Nullable Common.FlowProcessId flowId) {

        var processId = ciProcess(BACKEND_COMPONENT);

        var trunkRevision = createAndPrepareConfiguration();
        var branch = createBranch(processId, trunkRevision).getArcBranch();

        var request = FrontendReleaseApi.StartReleaseRequest.newBuilder()
                .setReleaseProcessId(ProtoMappers.toProtoReleaseProcessId(processId))
                .setCommit(ProtoMappers.toCommitId(trunkRevision))
                .setBranch(branch.asString())
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(trunkRevision));

        if (flowId != null) {
            request.setFlowProcessId(flowId);
        }
        //.setFlowType(Common.FlowType.FT_HOTFIX) - пока ставить необязательно

        return grpcService.startRelease(request.build());
    }


    private FrontendReleaseApi.StartReleaseRequest startReleaseRequest(
            Common.ReleaseProcessId processId,
            int revisionNumber
    ) {
        return FrontendReleaseApi.StartReleaseRequest.newBuilder()
                .setReleaseProcessId(processId)
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(revision(revisionNumber)))
                .build();
    }

    private FrontendReleaseApi.GetReleaseFlowsRequest getReleaseFlowsRequest(
            Common.ReleaseProcessId processId, OrderedArcRevision configRevision) {
        return FrontendReleaseApi.GetReleaseFlowsRequest.newBuilder()
                .setReleaseProcessId(processId)
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(configRevision))
                .build();
    }

    private OrderedArcRevision createAndPrepareConfiguration() {
        return createAndPrepareConfiguration(START_REVISION, CONFIG_PATH);
    }

    private void prepareLaunch(FrontendReleaseApi.StartReleaseResponse release) {
        var launchId = ProtoMappers.toLaunchId(release.getReleaseLaunch().getId());
        db.currentOrTx(() -> {
            var launch = db.launches().get(launchId).toBuilder()
                    .flowLaunchId("xxx-temp")
                    .status(LaunchState.Status.SUCCESS)
                    .build();
            db.launches().save(launch);
        });

    }

    private static CiProcessId ciProcess(String id) {
        return CiProcessId.ofRelease(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), id);
    }

    private static Common.ReleaseProcessId release(String id) {
        return ProtoMappers.toProtoReleaseProcessId(ciProcess(id));
    }

    private static GetCommitsRequest forProcess(Common.ReleaseProcessId protoReleaseId) {
        return GetCommitsRequest.newBuilder()
                .setReleaseProcessId(protoReleaseId)
                .build();
    }

    private Branch createBranch(CiProcessId releaseId, OrderedArcRevision revision) {
        delegateToken(releaseId.getPath());
        return db.currentOrTx(() -> branchService.createBranch(releaseId, revision, TestData.CI_USER));
    }

}
