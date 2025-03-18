package ru.yandex.ci.api.controllers.frontend;

import java.time.Instant;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.timeline.BranchState;
import ru.yandex.ci.core.timeline.BranchVcsInfo;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.test.TestUtils.parseProtoText;


class ConfigStateBuilderTest extends EngineTestBase {

    public static final FlowFullId FLOW_FULL_ID = new FlowFullId("/my/path", "flow");

    @Autowired
    private BranchService branchService;

    @Test
    void getFlowsVisibleInActions() {
        assertThat(
                ConfigStateBuilder.getVisibleActions(List.of(
                        ActionConfigState.builder()
                                .flowId("show-in-actions-null")
                                .title("1")
                                .triggers(List.of())
                                .build(),
                        ActionConfigState.builder()
                                .flowId("show-in-actions-null-but-pr-trigger")
                                .title("2")
                                .triggers(List.of(TriggerConfig.builder()
                                        .on(TriggerConfig.On.PR)
                                        .flow("show-in-actions-null-but-pr-trigger")
                                        .build()))
                                .build(),
                        ActionConfigState.builder()
                                .flowId("show-in-actions-null-but-commit-trigger")
                                .title("3")
                                .triggers(List.of(TriggerConfig.builder()
                                        .on(TriggerConfig.On.COMMIT)
                                        .flow("show-in-actions-null-but-commit-trigger")
                                        .build()))
                                .build(),
                        ActionConfigState.builder()
                                .flowId("show-in-actions-true")
                                .title("4")
                                .showInActions(true)
                                .triggers(List.of())
                                .build(),
                        ActionConfigState.builder()
                                .flowId("has-triggers-but-show-in-actions-false")
                                .title("5")
                                .showInActions(false)
                                .triggers(List.of(TriggerConfig.builder()
                                        .on(TriggerConfig.On.PR)
                                        .flow("has-triggers-but-show-in-actions-false")
                                        .build()))
                                .build()
                ))
        ).extracting(ActionConfigState::getFlowId)
                .containsExactly(
                        "show-in-actions-null-but-pr-trigger",
                        "show-in-actions-null-but-commit-trigger",
                        "show-in-actions-true"
                );
    }

    @Test
    void shouldReturnReleaseAndFlowCounters() {
        var releaseProcessId = CiProcessId.ofRelease(FLOW_FULL_ID.getPath(), "flow");
        var flowProcessId = CiProcessId.ofFlow(FLOW_FULL_ID);

        var releaseBranch = ArcBranch.ofBranchName("releases/ci/1");
        var trunkRevision = OrderedArcRevision.fromRevision(ArcRevision.of("r1"), ArcBranch.trunk(), 1, -1);
        var branchRevision = OrderedArcRevision.fromRevision(ArcRevision.of("r1"), releaseBranch, 1, -1);

        var configState = ConfigState.builder()
                .configPath(FLOW_FULL_ID.getPath())
                .title("config title")
                .project("ci")
                .status(ConfigState.Status.OK)
                .release(ReleaseConfigState.builder()
                        .releaseId("flow")
                        .title("release title")
                        .releaseBranchesEnabled(true)
                        .build())
                .action(ActionConfigState.builder()
                        .flowId("flow")
                        .title("flow title")
                        .showInActions(true)
                        .build())
                .build();

        // test data
        db.currentOrTx(() -> {
            int number = 0;
            List.of(
                    createLaunch(releaseProcessId, ++number, Status.RUNNING, ArcBranch.trunk(), trunkRevision),
                    createLaunch(releaseProcessId, ++number, Status.RUNNING, ArcBranch.trunk(), trunkRevision),
                    // counters for release branch should not be mixed with trunk counters
                    createLaunch(releaseProcessId, ++number, Status.RUNNING, releaseBranch, branchRevision),
                    // counters for release should not be mixed with flow counters
                    createLaunch(releaseProcessId, ++number, Status.FAILURE, ArcBranch.trunk(), trunkRevision),
                    createLaunch(flowProcessId, ++number, Status.FAILURE, ArcBranch.trunk(), trunkRevision),
                    // we summarize counters across all branches for flows (not releases)
                    createLaunch(flowProcessId, ++number, Status.FAILURE, releaseBranch, branchRevision)
            ).forEach(db.launches()::save);

            var branch = db.branches().save(BranchInfo.builder()
                    .id(BranchInfo.Id.of(releaseBranch.getBranch()))
                    .baseRevision(branchRevision)
                    .commitId("r1")
                    .created(Instant.parse("2021-01-01T00:00:00Z"))
                    .createdBy("login")
                    .build());

            db.timelineBranchItems().save(
                    TimelineBranchItem.builder()
                            .idOf(releaseProcessId, branch.getArcBranch())
                            .state(BranchState.builder().build())
                            .vcsInfo(BranchVcsInfo.builder().build())
                            .version(Version.majorMinor("1", "1"))
                            .build()
            );
        });

        var configStates = new ConfigStateBuilder(db, autoReleaseService, branchService)
                .toProtoConfigStateByProject(List.of(configState), true);

        assertThat(configStates).isEqualTo(Map.of(
                "ci", List.of(
                        parseProtoText(
                                "project/ConfigStateBuilderTest.shouldReturnReleaseAndFlowCounters.pb",
                                Common.ConfigState.class
                        )
                )
        ));
    }

    private Launch createLaunch(CiProcessId processId, int number, Status status, ArcBranch branch,
                                OrderedArcRevision revision) {
        var launchId = LaunchId.of(processId, number);
        return Launch.builder()
                .project("ci")
                .launchId(launchId)
                .status(status)
                .vcsInfo(LaunchVcsInfo.builder()
                        .selectedBranch(branch)
                        .revision(revision)
                        .build())
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(OrderedArcRevision.fromRevision(
                                ArcRevision.of("r0"), ArcBranch.trunk(), 0, -1
                        ))
                        .flowId(FLOW_FULL_ID)
                        .runtimeInfo(LaunchRuntimeInfo.builder()
                                .sandboxOwner("CI")
                                .build())
                        .stageGroupId("my-stages")
                        .build())
                .version(Version.major(String.valueOf(launchId.getNumber())))
                .build();
    }


}
