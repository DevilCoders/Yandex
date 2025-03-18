package ru.yandex.ci.api.controllers.storage;

import java.nio.file.Path;
import java.util.List;

import io.grpc.ManagedChannel;
import io.grpc.StatusRuntimeException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.junit.jupiter.params.provider.ValueSource;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.StartFlowRequest;
import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi;
import ru.yandex.ci.api.internal.frontend.project.ProjectServiceGrpc;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.api.storage.StorageApi.ExtendedStartFlowRequest;
import ru.yandex.ci.api.storage.StorageFlowServiceGrpc;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.VirtualConfigState;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.launch.LaunchStartTask;
import ru.yandex.ci.engine.proto.ProtoMappers;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.test.TestUtils.parseProtoText;

class StorageFlowControllerTest extends ControllerTestBase<StorageFlowServiceGrpc.StorageFlowServiceBlockingStub> {

    private static final int START_REVISION = 99;
    private static final Path CONFIG_PATH = Path.of("autocheck");

    @Autowired
    private LaunchStartTask launchStartTask;

    private ProjectServiceGrpc.ProjectServiceBlockingStub projectStub;

    @BeforeEach
    void setUp() {
        projectStub = ProjectServiceGrpc.newBlockingStub(getChannel());
    }

    @Override
    protected StorageFlowServiceGrpc.StorageFlowServiceBlockingStub createStub(ManagedChannel channel) {
        return StorageFlowServiceGrpc.newBlockingStub(channel);
    }

    @ParameterizedTest
    @MethodSource("startFlowWithVars")
    void startFlowWithVarsTitle(String flowVars, String expectTitle) {
        var revision = createAndPrepareConfiguration();
        var processId = CiProcessId.ofFlow(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), "run-flow");

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setFlowTitle("Sample")
                .setFlowVars(flowVars)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId.get()));
        var job = flowLaunch.getJobState("execute-single-task");
        assertThat(job.getTitle())
                .isEqualTo(expectTitle);

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull(); // No delegated config
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        // Do not update discoveredCommit with Large test flow
        var commit = db.currentOrReadOnly(() -> db.discoveredCommit().findCommit(processId, revision));
        assertThat(commit).isNotEmpty();
        assertThat(commit.get().getState().getLaunchIds()).isEmpty();
    }

    @Test
    void startFlowWithDelegatedSecretsAndPostpone() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();
        verifyProject("storage/getProject-empty.pb");
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-empty.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_LARGE_TESTS,
                Common.VirtualProcessType.VP_NATIVE_BUILDS
        );
        verifyProjectInfo("storage/getProjectInfo-empty.pb");

        var revision = createAndPrepareConfiguration();

        var delegatedConfigPath = Path.of("simple-ci");
        var delegatedRevision = createAndPrepareConfiguration(100, delegatedConfigPath);

        var processId = CiProcessId.ofFlow(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), "run-flow");

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath(delegatedConfigPath.resolve("a.yaml").toString())
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(delegatedRevision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setDelegatedConfig(delegatedConfig)
                .setPostponed(true)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        assertThat(launch.getStatus()).isEqualTo(LaunchState.Status.POSTPONE);

        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isEqualTo("CI");
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();
        verifyProject("storage/getProject-normal.pb");
        verifyProject("storage/getProject-normal.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-normal.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_LARGE_TESTS,
                Common.VirtualProcessType.VP_NATIVE_BUILDS
        );
        verifyProjectInfo("storage/getProjectInfo-normal.pb");

        launchService.startDelayedOrPostponedLaunch(launchId.toKey());

        flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        assertThat(launch.getStatus()).isEqualTo(LaunchState.Status.RUNNING);

        var id = flowLaunchId.get();
        db.currentOrReadOnly(() -> db.flowLaunch().get(id));
    }

    @Test
    void startFlowWithDelegatedSecrets() {
        var ciProcessId = CiProcessId.ofFlow(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), "run-flow");
        // Both sandbox and native owners are ignored
        startFlowWithDelegatedSecrets(ciProcessId, "CI");
    }

    @Test
    void startFlowWithDelegatedSecretsForLargeTest() {
        var ciProcessId = CiProcessId.ofFlow(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), "run-flow");
        startFlowWithDelegatedSecrets(
                VirtualCiProcessId.toVirtual(ciProcessId, VirtualType.VIRTUAL_LARGE_TEST),
                "LARGE-CI"
        );
    }

    @Test
    void startFlowWithDelegatedSecretsForNativeBuild() {
        var ciProcessId = CiProcessId.ofFlow(CONFIG_PATH.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME), "run-flow");
        startFlowWithDelegatedSecrets(
                VirtualCiProcessId.toVirtual(ciProcessId, VirtualType.VIRTUAL_NATIVE_BUILD),
                "NATIVE-CI"
        );
    }

    private void startFlowWithDelegatedSecrets(CiProcessId processId, String expectSandboxOwner) {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();

        var revision = createAndPrepareConfiguration();

        var delegatedConfigPath = Path.of("simple-ci2");
        var delegatedRevision = createAndPrepareConfiguration(100, delegatedConfigPath);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath(delegatedConfigPath.resolve("a.yaml").toString())
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(delegatedRevision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setDelegatedConfig(delegatedConfig)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        assertThat(launch.getStatus()).isEqualTo(LaunchState.Status.RUNNING);
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isEqualTo(expectSandboxOwner);
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        var virtualCiProcessId = VirtualCiProcessId.of(processId);
        if (virtualCiProcessId.getVirtualType() == null) {
            assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();
        } else {
            assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isNotEmpty();
        }
        db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId.get()));
    }

    @Test
    void startFlowWithVirtualFlowLargeTest() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NATIVE_BUILDS);
        verifyProjectInfo("storage/getProjectInfo-empty.pb");

        var revision = createAndPrepareConfiguration();

        var baseProcessId = CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualType = VirtualType.VIRTUAL_LARGE_TEST;
        var processId = VirtualCiProcessId.toVirtual(baseProcessId, virtualType);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull();
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project(virtualType.getService())
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));
        verifyProject("storage/getProject-normal.pb");
        verifyProject("storage/getProject-normal.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-virtual.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NATIVE_BUILDS);
        verifyProject("storage/getProject-normal-virtual.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_LARGE_TESTS
        );
        verifyProject("storage/getProject-virtual-normal.pb",
                Common.VirtualProcessType.VP_LARGE_TESTS,
                Common.VirtualProcessType.VP_NONE
        );
        verifyProject("storage/getProject-normal-virtual.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_NATIVE_BUILDS,
                Common.VirtualProcessType.VP_LARGE_TESTS
        );
        verifyProjectInfo("storage/getProjectInfo-normal-virtual.pb");
    }

    @Test
    void startFlowWithVirtualFlowNativeBuild() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_NATIVE_BUILDS);
        verifyProjectInfo("storage/getProjectInfo-empty.pb");

        var revision = createAndPrepareConfiguration();

        var baseProcessId = CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualType = VirtualType.VIRTUAL_NATIVE_BUILD;
        var processId = VirtualCiProcessId.toVirtual(baseProcessId, virtualType);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull();
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project(virtualType.getService())
                        .title("Native builds ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));
        verifyProject("storage/getProject-normal.pb");
        verifyProject("storage/getProject-normal.pb", Common.VirtualProcessType.VP_NONE);
        verifyProject("storage/getProject-empty.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
        verifyProject("storage/getProject-virtual-native.pb", Common.VirtualProcessType.VP_NATIVE_BUILDS);
        verifyProject("storage/getProject-normal.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_LARGE_TESTS
        );
        verifyProject("storage/getProject-normal-virtual-native.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_NATIVE_BUILDS
        );
        verifyProject("storage/getProject-normal-virtual-native.pb",
                Common.VirtualProcessType.VP_NONE,
                Common.VirtualProcessType.VP_NATIVE_BUILDS,
                Common.VirtualProcessType.VP_LARGE_TESTS
        );
        verifyProjectInfo("storage/getProjectInfo-normal-virtual-native.pb");
    }

    @Test
    void startFlowWithVirtualFlowAndDelegatedSecretsToDefault() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();

        var revision = createAndPrepareConfiguration();

        var baseProcessId = CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualType = VirtualType.VIRTUAL_LARGE_TEST;
        assertThat(virtualType.getService())
                .isNotEqualTo("ci");

        var processId = VirtualCiProcessId.toVirtual(baseProcessId, virtualType);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var delegatedConfigPath = Path.of("simple-ci");
        var delegatedRevision = createAndPrepareConfiguration(100, delegatedConfigPath);
        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath(delegatedConfigPath.resolve("a.yaml").toString())
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(delegatedRevision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setDelegatedConfig(delegatedConfig)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        assertThat(launch.getProject())
                .isEqualTo("ci");

        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isEqualTo("CI");
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project("ci")
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));

        var response2 = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .build());
        var launchId2 = LaunchId.of(processId, response2.getLaunch().getNumber());
        var launch2 = db.currentOrReadOnly(() -> db.launches().get(launchId2));
        assertThat(launch2.getProject())
                .isEqualTo(virtualType.getService());
        assertThat(response2.getLaunch().getNumber())
                .isGreaterThan(response.getLaunch().getNumber());
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project(virtualType.getService())
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));
    }


    @Test
    void startFlowWithVirtualFlowAndDefaultToDelegatedSecrets() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();

        var revision = createAndPrepareConfiguration();

        var baseProcessId = CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualType = VirtualType.VIRTUAL_LARGE_TEST;
        var processId = VirtualCiProcessId.toVirtual(baseProcessId, virtualType);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull();
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .project(virtualType.getService())
                        .virtualType(virtualType)
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));


        var delegatedConfigPath = Path.of("simple-ci");
        var delegatedRevision = createAndPrepareConfiguration(100, delegatedConfigPath);
        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath(delegatedConfigPath.resolve("a.yaml").toString())
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(delegatedRevision))
                .build();


        var response2 = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setDelegatedConfig(delegatedConfig)
                .build());

        var launchId2 = LaunchId.of(processId, response2.getLaunch().getNumber());
        var launch2 = db.currentOrReadOnly(() -> db.launches().get(launchId2));
        var runtimeInfo2 = launch2.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo2.getSandboxOwner()).isEqualTo("CI");
        assertThat(runtimeInfo2.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo2.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        assertThat(response2.getLaunch().getNumber())
                .isGreaterThan(response.getLaunch().getNumber());
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project("ci")
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));
    }

    @Test
    void testGetLastValidConfig() {
        var revision = createAndPrepareConfiguration();

        var request = StorageApi.GetLastValidConfigRequest.newBuilder()
                .setDir(CONFIG_PATH.toString())
                .setBranch("trunk")
                .build();
        var response = grpcService.getLastValidConfig(request);

        assertThat(response)
                .isEqualTo(StorageApi.GetLastValidConfigResponse.newBuilder()
                        .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                        .build());
    }

    @Test
    void testGetLastValidConfigNotFound() {
        var request = StorageApi.GetLastValidConfigRequest.newBuilder()
                .setDir("some/config")
                .setBranch("trunk")
                .build();

        //noinspection ResultOfMethodCallIgnored
        assertThatThrownBy(() -> grpcService.getLastValidConfig(request))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: Unable to find last valid configuration for some/config/a.yaml at trunk");
    }

    @Test
    void testGetLastValidPrefixedConfigEmpty() {
        var request = StorageApi.GetLastValidPrefixedConfigRequest.newBuilder()
                .setRevision(START_REVISION)
                .build();

        var response = grpcService.getLastValidPrefixedConfig(request);
        assertThat(response)
                .isEqualTo(StorageApi.GetLastValidPrefixedConfigResponse.getDefaultInstance());
    }

    @ParameterizedTest
    @ValueSource(ints = {99, 100})
    void testGetLastValidPrefixedConfigDefault(int revision) {
        var config1 = "sample/dir";
        var config2 = "sample/dir/subdir";
        var config3 = "sample/dir/subdir/noci";
        var config4 = "sample/dir/subdir/nosecret";
        var rev2 = createAndPrepareConfiguration(97, true, Path.of(config1));
        var rev3 = createAndPrepareConfiguration(98, true, Path.of(config2));
        createAndPrepareConfiguration(99, false, Path.of(config3), Path.of(config4)); // no config

        var matched1 = List.of("sample/dir", "sample/dir/a", "sample/dir/b");
        var matched2 = List.of("sample/dir/subdir", "sample/dir/subdir/a");
        var matched3 = List.of(
                "sample/dir/subdir/noci",
                "sample/dir/subdir/noci/a",
                "sample/dir/subdir/nosecret",
                "sample/dir/subdir/nosecret/a"
        );
        var unmatched = List.of("sample", "sample/a");

        var request = StorageApi.GetLastValidPrefixedConfigRequest.newBuilder()
                .setRevision(revision)
                .addAllPrefixDir(matched1)
                .addAllPrefixDir(matched2)
                .addAllPrefixDir(matched3)
                .addAllPrefixDir(unmatched)
                .build();

        var response = grpcService.getLastValidPrefixedConfig(request);

        var expectPath1 = config1 + "/a.yaml";
        var expectRev2 = ProtoMappers.toProtoOrderedArcRevision(rev2);

        var expectPath2 = config2 + "/a.yaml";
        var expectRev3 = ProtoMappers.toProtoOrderedArcRevision(rev3);

        var expectResponse = StorageApi.GetLastValidPrefixedConfigResponse.newBuilder();
        for (var prefix : matched1) {
            expectResponse.addResponsesBuilder()
                    .setPrefixDir(prefix)
                    .setPath(expectPath1)
                    .setConfigRevision(expectRev2);
        }
        for (var prefix : matched2) {
            expectResponse.addResponsesBuilder()
                    .setPrefixDir(prefix)
                    .setPath(expectPath2)
                    .setConfigRevision(expectRev3);
        }
        for (var prefix : matched3) {
            expectResponse.addResponsesBuilder()
                    .setPrefixDir(prefix)
                    .setPath(expectPath2)
                    .setConfigRevision(expectRev3);
        }

        assertThat(response)
                .isEqualTo(expectResponse.build());
    }

    @Test
    void testGetLastValidPrefixedConfigDefaultUnmatched() {
        var config = "sample/dir";
        createAndPrepareConfiguration(99, Path.of(config));

        var request = StorageApi.GetLastValidPrefixedConfigRequest.newBuilder()
                .setRevision(98)
                .addPrefixDir(config)
                .build();

        var response = grpcService.getLastValidPrefixedConfig(request);
        assertThat(response)
                .isEqualTo(StorageApi.GetLastValidPrefixedConfigResponse.getDefaultInstance());
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Test
    void testMarkDiscoveredUnknownCommit() {
        var revision = revision(START_REVISION);
        assertThat(db.currentOrReadOnly(() -> db.commitDiscoveryProgress().find(revision.getCommitId())))
                .isEmpty();
        var commit = Common.CommitId.newBuilder()
                .setCommitId(revision.getCommitId())
                .build();

        var arcCommitId = ArcCommit.Id.of(revision.getCommitId());
        assertThatThrownBy(() -> grpcService.markDiscoveredCommit(StorageApi.MarkDiscoveredCommitRequest.newBuilder()
                .setRevision(commit)
                .build()))
                .isInstanceOf(StatusRuntimeException.class)
                .hasMessage("NOT_FOUND: Unable to find key [%s] in table [main/Commit]".formatted(arcCommitId));
    }

    @Test
    void testMarkDiscoveredCommit() {
        var revision = createAndPrepareConfiguration();

        var commit = Common.CommitId.newBuilder()
                .setCommitId(revision.getCommitId())
                .build();
        var response = grpcService.markDiscoveredCommit(StorageApi.MarkDiscoveredCommitRequest.newBuilder()
                .setRevision(commit)
                .build());
        assertThat(response)
                .isEqualTo(StorageApi.MarkDiscoveredCommitResponse.getDefaultInstance());

        var progress = db.currentOrReadOnly(() -> db.commitDiscoveryProgress().find(revision.getCommitId()))
                .orElseThrow();
        assertThat(progress)
                .isEqualTo(CommitDiscoveryProgress.builder()
                        .arcRevision(revision)
                        .dirDiscoveryFinished(true)
                        .storageDiscoveryFinished(true)
                        .build());
    }

    @Test
    void startWithTestReference() {
        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll())).isEmpty();

        var revision = createAndPrepareConfiguration();

        var baseProcessId = CiProcessId.ofFlow(Path.of("ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualType = VirtualType.VIRTUAL_LARGE_TEST;
        var processId = VirtualCiProcessId.toVirtual(baseProcessId, virtualType);

        var request = StartFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                .setBranch("trunk")
                .setRevision(ProtoMappers.toCommitId(revision))
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(revision))
                .build();

        var flowVars = """
                {
                    "testInfo": {
                        "suiteId": "9223372036854775808",
                        "toolchain": "chain-1"
                    }
                }""";

        var response = grpcService.startFlow(ExtendedStartFlowRequest.newBuilder()
                .setRequest(request)
                .setFlowVars(flowVars)
                .build());

        var launchId = LaunchId.of(processId, response.getLaunch().getNumber());
        var flowLaunchId = launchStartTask.acquireMutexAndExecute(launchId);
        assertThat(flowLaunchId).isNotEmpty();

        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull(); // No delegated config
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("REVIEW-CHECK-FAT"));
        assertThat(runtimeInfo.getYavTokenUid()).isEqualTo(YavToken.Id.of("tkn-uid"));

        // Do not update discoveredCommit with Large test flow
        var commit = db.currentOrReadOnly(() -> db.discoveredCommit().findCommit(processId, revision));
        assertThat(commit).isNotEmpty();
        assertThat(commit.get().getState().getLaunchIds()).isEmpty();

        assertThat(db.currentOrReadOnly(() -> db.virtualConfigStates().findAll()))
                .isEqualTo(List.of(VirtualConfigState.builder()
                        .id(VirtualConfigState.Id.of(processId.getPath()))
                        .virtualType(virtualType)
                        .project("autocheck")
                        .title("Large tests ci/demo-project/large-tests2")
                        .action(ActionConfigState.builder()
                                .flowId(baseProcessId.getSubId())
                                .title(baseProcessId.getSubId())
                                .showInActions(Boolean.TRUE)
                                .testId(ActionConfigState.TestId.of("9223372036854775808", "chain-1"))
                                .build()
                        )
                        .created(clock.instant())
                        .updated(clock.instant())
                        .status(ConfigState.Status.OK)
                        .build()));


        verifyProject("storage/getProject-virtual-with-test.pb", Common.VirtualProcessType.VP_LARGE_TESTS);
    }

    private OrderedArcRevision createAndPrepareConfiguration() {
        return createAndPrepareConfiguration(START_REVISION, CONFIG_PATH);
    }

    private void verifyProject(String expectProto, Common.VirtualProcessType... virtualProcessTypes) {
        var getProjectResponse = projectStub.getProject(FrontendProjectApi.GetProjectRequest.newBuilder()
                .setProjectId("autocheck")
                .addAllVirtualProcessType(List.of(virtualProcessTypes))
                .build());
        assertThat(getProjectResponse)
                .isEqualTo(parseProtoText(expectProto, FrontendProjectApi.GetProjectResponse.class));
    }

    private void verifyProjectInfo(String expectProto) {
        var getProjectResponse = projectStub.getProjectInfo(FrontendProjectApi.GetProjectInfoRequest.newBuilder()
                .setProjectId("autocheck")
                .setIncludeInvalidConfigs(true)
                .build());
        assertThat(getProjectResponse)
                .isEqualTo(parseProtoText(expectProto, FrontendProjectApi.GetProjectInfoResponse.class));
    }

    static List<Arguments> startFlowWithVars() {
        var simpleKV = """
                {
                    "title": "My Primary Task"
                }""";
        return List.of(
                Arguments.of(simpleKV, "My Primary Task"),
                Arguments.of("", "Title"));
    }
}
