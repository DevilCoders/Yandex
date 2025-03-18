package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;

import ru.yandex.arc.api.Repo;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowDescription;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchUserData;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.event.LaunchEventTask;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.LaunchStartTask;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class DiscoveryServiceOnCommitTriggerTest extends EngineTestBase {
    private final LaunchRuntimeInfo ciOnlyRuntime = LaunchRuntimeInfo.ofRuntimeSandboxOwner("CI");

    @BeforeEach
    public void setUp() {
        mockYav();
        mockValidationSuccessful();
    }

    @Test
    void processPostCommit_trunkCommitShouldStartFlow_whenConfigContainsTriggerOnCommit()
            throws YavDelegationException {
        Path configPath = TestData.TRIGGER_ON_COMMIT_AYAML_PATH;
        OrderedArcRevision configRevision = TestData.TRUNK_R11;

        CiProcessId processIdTrunk = TestData.ON_COMMIT_INTO_TRUNK_PROCESS_ID;
        CiProcessId processIdAny = TestData.ON_COMMIT_WOODCUTTER_PROCESS_ID;

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), configRevision.toRevision(), false);

        var expectDelayed = List.of(LaunchId.of(processIdTrunk, 1).toKey(), LaunchId.of(processIdAny, 1).toKey());
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(expectDelayed);

        assertThat(db.currentOrReadOnly(() -> db.launches().find(Set.copyOf(expectDelayed))))
                .isEqualTo(List.of(
                        Launch.builder()
                                .launchId(LaunchId.of(processIdTrunk, 1))
                                .title("OnCommit Woodcutter #1")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(configRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processIdTrunk))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(configRevision)
                                                .previousRevision(TestData.TRUNK_R10)
                                                .commit(TestData.TRUNK_COMMIT_11)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("1"))
                                .build(),
                        Launch.builder()
                                .launchId(LaunchId.of(processIdAny, 1))
                                .title("OnCommit Woodcutter #1")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(configRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processIdAny))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(configRevision)
                                                .commit(TestData.TRUNK_COMMIT_11)
                                                .previousRevision(TestData.TRUNK_R10)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("1"))
                                .build()
                ));

        abcServiceStub.addService(Abc.CI, "albazh");
        securityDelegationService.delegateYavTokens(
                configurationService.getConfig(configPath, configRevision),
                TestData.USER_TICKET,
                "albazh"
        );

        launchService.startDelayedLaunches(configPath, configRevision.toRevision());
        verify(bazingaTaskManagerStub, times(2)).schedule(Mockito.any(LaunchStartTask.class));
    }

    @Test
    void processPostCommit_releaseBranchCommitShouldStartFlow_whenConfigContainsTriggerOnCommit()
            throws YavDelegationException {
        Path configPath = TestData.TRIGGER_ON_COMMIT_AYAML_PATH;
        CiProcessId processId = TestData.ON_COMMIT_WOODCUTTER_PROCESS_ID;
        ArcBranch releaseBranch = ArcBranch.ofBranchName("releases/ci/on-commit-trigger-test");
        OrderedArcRevision revisionAtReleaseBranch = OrderedArcRevision.fromHash(
                releaseBranch + "/r1", releaseBranch, 1, 0
        );

        var commit = TestData.toBranchCommit(revisionAtReleaseBranch, "albazh");
        arcServiceStub.addCommit(
                commit,
                TestData.TRUNK_COMMIT_11,
                Map.of(configPath.getParent().resolve("readme.md"), Repo.ChangelistResponse.ChangeType.Modify)
        );

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), TestData.TRUNK_R11.toRevision(), false);
        abcServiceStub.addService(Abc.CI, "albazh");
        securityDelegationService.delegateYavTokens(
                configurationService.getConfig(configPath, TestData.TRUNK_R11),
                TestData.USER_TICKET,
                "albazh"
        );

        verify(bazingaTaskManagerStub, times(3)).schedule(Mockito.any(LaunchEventTask.class)); // +Flow from root
        discoveryServicePostCommits.processPostCommit(releaseBranch, revisionAtReleaseBranch.toRevision(), false);

        var activeLaunches = db.currentOrReadOnly(() -> db.launches().getActiveLaunches(processId));
        assertThat(activeLaunches).hasSize(2);

        assertThat(activeLaunches.get(0))
                .isEqualTo(
                        Launch.builder()
                                .launchId(LaunchId.of(processId, 2))
                                .title("OnCommit Woodcutter #2")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(TestData.TRUNK_R11)
                                                .flowId(FlowFullId.ofFlowProcessId(processId))
                                                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(
                                                        TestData.YAV_TOKEN_UUID, "CI"))
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.STARTING)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(revisionAtReleaseBranch)
                                                .commit(commit.withParents(List.of("r11")))
                                                .previousRevision(TestData.TRUNK_R11)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("2"))
                                .build()
                );

        // Запустится ещё и on-commit-into-release

        var captor = ArgumentCaptor.forClass(OnetimeTask.class);
        verify(bazingaTaskManagerStub, times(7)).schedule(captor.capture());

        var args = captor.getAllValues().stream()
                .filter(task -> task instanceof LaunchStartTask) // + триггер на коммит в релизный бранч
                .map(task -> (LaunchStartTask.Params) task.getParameters())
                .map(LaunchStartTask.Params::getLaunchId)
                .collect(Collectors.toList());
        assertThat(args)
                .isEqualTo(List.of(
                        new LaunchId(processId, 2),
                        new LaunchId(TestData.ON_COMMIT_INTO_RELEASE_PROCESS_ID, 1)
                ));
    }

    @Test
    void processPostCommit_trunkCommitShouldStartFlow_whenConfigContainsDiscoveryDirs()
            throws YavDelegationException {
        Path otherConfigPath = TestData.TRIGGER_ON_COMMIT_AYAML_PATH;

        Path configPath = TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH;
        OrderedArcRevision configRevision = TestData.TRUNK_R12;
        OrderedArcRevision changeRevision = TestData.TRUNK_R13;

        CiProcessId processIdNoFilter = TestData.DISCOVERY_DIR_NO_FILTER_ON_COMMIT_WOODCUTTER_PROCESS_ID;
        CiProcessId processId = TestData.DISCOVERY_DIR_ON_COMMIT_WOODCUTTER_PROCESS_ID;

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), configRevision.toRevision(), false);

        var launchNoFilter = Launch.builder()
                .launchId(LaunchId.of(processIdNoFilter, 1))
                .title("OnCommit Woodcutter without filters #1")
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .project("ci")
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(configRevision)
                                .flowId(FlowFullId.ofFlowProcessId(processIdNoFilter))
                                .runtimeInfo(ciOnlyRuntime)
                                .flowDescription(
                                        new LaunchFlowDescription(
                                                "OnCommit Woodcutter without filters",
                                                "OnCommit Woodcutter Flow without filters",
                                                Common.FlowType.FT_DEFAULT,
                                                null
                                        )
                                )
                                .build()
                )
                .hasDisplacement(false)
                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                .created(NOW)
                .activityChanged(NOW)
                .status(LaunchState.Status.DELAYED)
                .statusText("")
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(configRevision)
                                .commit(TestData.TRUNK_COMMIT_12)
                                .previousRevision(TestData.TRUNK_R11)
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .version(Version.major("1"))
                .build();

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(List.of(launchNoFilter.getId()));

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), changeRevision.toRevision(), false);

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();

        var expectDelayed = List.of(launchNoFilter.getId(), LaunchId.of(processId, 1).toKey());
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(expectDelayed);

        assertThat(db.currentOrReadOnly(() -> db.launches().find(Set.copyOf(expectDelayed))))
                .isEqualTo(List.of(
                        launchNoFilter,
                        Launch.builder()
                                .launchId(LaunchId.of(processId, 1))
                                .title("OnCommit Woodcutter #1")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(configRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processId))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(changeRevision)
                                                .commit(TestData.TRUNK_COMMIT_13)
                                                .previousRevision(TestData.TRUNK_R12)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("1"))
                                .build()
                ));
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, changeRevision)))
                .isEqualTo(List.of());

        abcServiceStub.addService(Abc.CI, "albazh");
        securityDelegationService.delegateYavTokens(
                configurationService.getConfig(configPath, configRevision),
                TestData.USER_TICKET,
                "albazh"
        );

        launchService.startDelayedLaunches(configPath, configRevision.toRevision());
        verify(bazingaTaskManagerStub, times(2)).schedule(Mockito.any(LaunchStartTask.class));
    }

    @Test
    void processPostCommit_trunkCommitShouldStartFlow_whenConfigContainsDiscoveryDirsExactPath()
            throws YavDelegationException {
        Path otherConfigPath = TestData.TRIGGER_ON_COMMIT_AYAML_PATH;

        Path configPath = TestData.DISCOVERY_DIR_ON_COMMIT2_AYAML_PATH;
        OrderedArcRevision configRevision = TestData.TRUNK_R12;
        OrderedArcRevision changeRevision = TestData.TRUNK_R13;

        CiProcessId processIdNoFilter = TestData.DISCOVERY_DIR_NO_FILTER_ON_COMMIT2_WOODCUTTER_PROCESS_ID;
        CiProcessId processId = TestData.DISCOVERY_DIR_ON_COMMIT2_WOODCUTTER_PROCESS_ID;

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), configRevision.toRevision(), false);

        var launchNoFilter = Launch.builder()
                .launchId(LaunchId.of(processIdNoFilter, 1))
                .title("OnCommit Woodcutter without filters #1")
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .project("ci")
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(configRevision)
                                .flowId(FlowFullId.ofFlowProcessId(processIdNoFilter))
                                .runtimeInfo(ciOnlyRuntime)
                                .flowDescription(
                                        new LaunchFlowDescription(
                                                "OnCommit Woodcutter without filters",
                                                "OnCommit Woodcutter Flow without filters",
                                                Common.FlowType.FT_DEFAULT,
                                                null
                                        )
                                )
                                .build()
                )
                .hasDisplacement(false)
                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                .created(NOW)
                .activityChanged(NOW)
                .status(LaunchState.Status.DELAYED)
                .statusText("")
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(configRevision)
                                .commit(TestData.TRUNK_COMMIT_12)
                                .previousRevision(TestData.TRUNK_R11)
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .version(Version.major("1"))
                .build();

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(List.of(launchNoFilter.getId()));

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), changeRevision.toRevision(), false);

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();

        var expectDelayed = List.of(launchNoFilter.getId(), LaunchId.of(processId, 1).toKey());
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(expectDelayed);
        assertThat(db.currentOrReadOnly(() -> db.launches().find(Set.copyOf(expectDelayed))))
                .isEqualTo(List.of(
                        launchNoFilter,
                        Launch.builder()
                                .launchId(LaunchId.of(processId, 1))
                                .title("OnCommit Woodcutter #1")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(configRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processId))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(changeRevision)
                                                .commit(TestData.TRUNK_COMMIT_13)
                                                .previousRevision(TestData.TRUNK_R12)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("1"))
                                .build()
                ));
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, changeRevision)))
                .isEqualTo(List.of());

        abcServiceStub.addService(Abc.CI, "albazh");
        securityDelegationService.delegateYavTokens(
                configurationService.getConfig(configPath, configRevision),
                TestData.USER_TICKET,
                "albazh"
        );

        launchService.startDelayedLaunches(configPath, configRevision.toRevision());
        verify(bazingaTaskManagerStub, times(2)).schedule(Mockito.any(LaunchStartTask.class));
    }


    @SuppressWarnings("MethodLength")
    @Test
    void processPostCommit_trunkCommitShouldStartFlow_whenConfigContainsDiscoveryDirsChangedInSameCommit()
            throws YavDelegationException {
        Path otherConfigPath = TestData.TRIGGER_ON_COMMIT_AYAML_PATH;

        Path configPath = TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH;
        OrderedArcRevision configRevision = TestData.TRUNK_R12;
        OrderedArcRevision changeRevision = TestData.TRUNK_R14;

        CiProcessId processIdNoFilter = TestData.DISCOVERY_DIR_NO_FILTER_ON_COMMIT_WOODCUTTER_PROCESS_ID;
        CiProcessId processId = TestData.DISCOVERY_DIR_ON_COMMIT_WOODCUTTER_PROCESS_ID;

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), configRevision.toRevision(), false);

        var launchNoFilter = Launch.builder()
                .launchId(LaunchId.of(processIdNoFilter, 1))
                .title("OnCommit Woodcutter without filters #1")
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .project("ci")
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(configRevision)
                                .flowId(FlowFullId.ofFlowProcessId(processIdNoFilter))
                                .runtimeInfo(ciOnlyRuntime)
                                .flowDescription(
                                        new LaunchFlowDescription(
                                                "OnCommit Woodcutter without filters",
                                                "OnCommit Woodcutter Flow without filters",
                                                Common.FlowType.FT_DEFAULT,
                                                null
                                        )
                                )
                                .build()
                )
                .hasDisplacement(false)
                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                .created(NOW)
                .activityChanged(NOW)
                .status(LaunchState.Status.DELAYED)
                .statusText("")
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(configRevision)
                                .commit(TestData.TRUNK_COMMIT_12)
                                .previousRevision(TestData.TRUNK_R11)
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .version(Version.major("1"))
                .build();

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(List.of(launchNoFilter.getId()));

        discoveryServicePostCommits.processPostCommit(ArcBranch.trunk(), changeRevision.toRevision(), false);

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(otherConfigPath, configRevision)))
                .isEmpty();

        // Config was changes, new launch started with config = changeRevision
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(List.of(launchNoFilter.getId()));

        var expectDelayed = List.of(LaunchId.of(processIdNoFilter, 2).toKey(), LaunchId.of(processId, 1).toKey());
        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, changeRevision)))
                .isEqualTo(expectDelayed);
        assertThat(db.currentOrReadOnly(() -> db.launches().find(Set.copyOf(expectDelayed))))
                .isEqualTo(List.of(
                        Launch.builder()
                                .launchId(LaunchId.of(processIdNoFilter, 2))
                                .title("OnCommit Woodcutter without filters #2")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(changeRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processIdNoFilter))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter without filters",
                                                                "OnCommit Woodcutter Flow without filters",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(changeRevision)
                                                .commit(TestData.TRUNK_COMMIT_14)
                                                .previousRevision(TestData.TRUNK_R13)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("2"))
                                .build(),
                        Launch.builder()
                                .launchId(LaunchId.of(processId, 1))
                                .title("OnCommit Woodcutter #1")
                                .type(Launch.Type.USER)
                                .notifyPullRequest(false)
                                .project("ci")
                                .flowInfo(
                                        LaunchFlowInfo.builder()
                                                .configRevision(changeRevision)
                                                .flowId(FlowFullId.ofFlowProcessId(processId))
                                                .runtimeInfo(ciOnlyRuntime)
                                                .flowDescription(
                                                        new LaunchFlowDescription(
                                                                "OnCommit Woodcutter",
                                                                "OnCommit Woodcutter Flow",
                                                                Common.FlowType.FT_DEFAULT,
                                                                null
                                                        )
                                                )
                                                .build()
                                )
                                .hasDisplacement(false)
                                .triggeredBy(UserUtils.loginForInternalCiProcesses())
                                .created(NOW)
                                .activityChanged(NOW)
                                .status(LaunchState.Status.DELAYED)
                                .statusText("")
                                .vcsInfo(
                                        LaunchVcsInfo.builder()
                                                .revision(changeRevision)
                                                .commit(TestData.TRUNK_COMMIT_14)
                                                .previousRevision(TestData.TRUNK_R13)
                                                .build()
                                )
                                .userData(new LaunchUserData(List.of(), false))
                                .version(Version.major("1"))
                                .build()
                ));

        abcServiceStub.addService(Abc.CI, "albazh");
        for (var rev : Set.of(configRevision, changeRevision)) {
            securityDelegationService.delegateYavTokens(
                    configurationService.getConfig(configPath, rev),
                    TestData.USER_TICKET,
                    "albazh"
            );
        }

        launchService.startDelayedLaunches(configPath, configRevision.toRevision());
        verify(bazingaTaskManagerStub).schedule(Mockito.any(LaunchStartTask.class));

        // +2 tasks
        launchService.startDelayedLaunches(configPath, changeRevision.toRevision());
        verify(bazingaTaskManagerStub, times(3)).schedule(Mockito.any(LaunchStartTask.class));
    }

}
