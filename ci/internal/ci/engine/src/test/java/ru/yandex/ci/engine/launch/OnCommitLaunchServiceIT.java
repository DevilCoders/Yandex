package ru.yandex.ci.engine.launch;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.launch.OnCommitLaunchService.StartFlowParameters;
import ru.yandex.lang.NonNullApi;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.verify;

@NonNullApi
public class OnCommitLaunchServiceIT extends EngineTestBase {

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
        clearInvocations(bazingaTaskManagerStub);
        mockYav();
        mockArcanumGetReviewRequestData();
    }


    @Test
    void startFlowOnTrunk() {
        CiProcessId processId = TestData.SIMPLE_FLOW_PROCESS_ID;
        discoveryToR2();
        delegateToken(processId.getPath());

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(ArcBranch.trunk())
                        .revision(TestData.TRUNK_R2.toRevision())
                        .configOrderedRevision(TestData.TRUNK_R2)
                        .triggeredBy("user-login")
                        .build()
        );

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
    }

    @Test
    void startFlowOnTrunkEmptyConfigRevision() {
        CiProcessId processId = TestData.SIMPLE_FLOW_PROCESS_ID;
        discoveryToR2();
        delegateToken(processId.getPath());

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(ArcBranch.trunk())
                        .revision(TestData.TRUNK_R2.toRevision())
                        .triggeredBy("user-login")
                        .build()
        );

        assertThat(launch.getConfigCommitId()).isEqualTo(TestData.TRUNK_R2.getCommitId());

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
    }

    @Test
    void customActionRuntime() {
        CiProcessId processId = CiProcessId.ofFlow(
                TestData.CONFIG_PATH_ACTION_CUSTOM_RUNTIME, "first"
        );

        discoveryToR2();
        delegateToken(processId.getPath());

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(ArcBranch.trunk())
                        .revision(TestData.TRUNK_R2.toRevision())
                        .configOrderedRevision(TestData.TRUNK_R2)
                        .triggeredBy("user-login")
                        .build()
        );

        var runtime = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtime.getSandboxOwner()).isNull();
        assertThat(runtime.getRuntimeConfig()).isEqualTo(RuntimeConfig.ofSandboxOwner("CI-first"));
    }

    @Test
    void startFlowOnReleaseBranch() {
        CiProcessId processId = TestData.SIMPLE_FLOW_PROCESS_ID;
        discoveryToR2();
        delegateToken(processId.getPath());

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(TestData.RELEASE_BRANCH_2)
                        .revision(TestData.RELEASE_BRANCH_COMMIT_6_3.getRevision())
                        .configOrderedRevision(TestData.TRUNK_R2)
                        .triggeredBy("user-login")
                        .build()
        );

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
    }

    @Test
    void startFlowOnPrMergeRevision() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_PR_NEW, "sawmill");
        ArcBranch branch = ArcBranch.ofPullRequest(42);

        discoveryToR4();
        initDiffSets();
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_1);
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_3);

        delegateToken(processId.getPath(), branch);

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(branch)
                        .revision(TestData.DIFF_SET_3.getOrderedMergeRevision().toRevision())
                        .configOrderedRevision(TestData.DIFF_SET_3.getOrderedMergeRevision())
                        .triggeredBy("user-login")
                        .build()
        );

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
    }

    @Test
    void startFlowOnPrMergeRevisionWhenConfigHasSecurityProblems() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_PR_NEW, "sawmill");
        ArcBranch branch = ArcBranch.ofPullRequest(42);

        discoveryToR4();
        initDiffSets();
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_1);
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_3);

        Assertions.assertThatThrownBy(() ->
                        onCommitLaunchService.startFlow(
                                StartFlowParameters.builder()
                                        .processId(processId)
                                        .branch(branch)
                                        .revision(TestData.DIFF_SET_3.getOrderedMergeRevision().toRevision())
                                        .configOrderedRevision(TestData.DIFF_SET_3.getOrderedMergeRevision())
                                        .triggeredBy("user-login")
                                        .build()
                        )
                ).isInstanceOf(IllegalArgumentException.class)
                .hasMessage(
                        "Config pr/new/a.yaml at revision OrderedArcRevision(commitId=ds3, branch=pr:42, number=3, " +
                                "pullRequestId=42) has status SECURITY_PROBLEM and can't be used"
                );
    }

    @Test
    void startFlowOnUserBranch() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_SIMPLE_RELEASE, "sawmill");

        discoveryToR4();
        initDiffSets();
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_1);
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_3);
        delegateToken(processId.getPath());

        Launch launch = onCommitLaunchService.startFlow(
                StartFlowParameters.builder()
                        .processId(processId)
                        .branch(TestData.USER_BRANCH)
                        .revision(TestData.THIRD_REVISION_AT_USER_BRANCH.toRevision())
                        .configOrderedRevision(TestData.TRUNK_R2)
                        .triggeredBy("user-login")
                        .build()
        );

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
    }

}
