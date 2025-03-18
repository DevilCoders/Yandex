package ru.yandex.ci.engine.launch;

import java.time.Instant;
import java.util.List;

import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchUserData;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.lang.NonNullApi;

@ContextConfiguration(classes = {
        LaunchStateSynchronizer.class
})
@NonNullApi
@MockBean(XivaNotifier.class)
public class LaunchStateSynchronizerIT extends YdbCiTestBase {

    @SuppressWarnings("UnusedVariable")
    @MockBean
    private PullRequestService pullRequestService;

    @SuppressWarnings("UnusedVariable")
    @MockBean
    private BranchService branchService;

    @SuppressWarnings("UnusedVariable")
    @MockBean
    private EventPublisher eventPublisher;

    @MockBean
    @SuppressWarnings("UnusedVariable")
    private CommitRangeService commitRangeService;

    @MockBean
    @SuppressWarnings("UnusedVariable")
    private BazingaTaskManager bazingaTaskManager;

    @Autowired
    private LaunchStateSynchronizer launchStateSynchronizer;


    @DisplayName("flowLaunchUpdated should not fail when Launch table updates twice in one YdbTransaction")
    @Test
    void testDoubleUpdate() {
        /* See CI-1120. We test next exceptions:

        1) com.yandex.ydb.core.UnexpectedResultException: not successful result, code: GENERIC_ERROR, issues: [#1060
        Execution (S_ERROR)
        #2500 Detected violation of logical DML constraints. YDB transactions don't see their own changes, make sure
        you perform all table reads before any modifications. (S_ERROR)
        1:4 - 1:4: #2500 Data modifications previously made to table '/local/Launch' in current transaction won't be
        seen by operation: 'Select' (S_WARNING)]

        2) com.yandex.ydb.core.UnexpectedResultException: not successful result, code: GENERIC_ERROR, issues: [#1060
        Execution (S_ERROR)
           13:17 - 13:17: #2008 Multiple modification of table with secondary indexes is not supported yet (S_ERROR)]
        */
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        LaunchId launchId = LaunchId.of(processId, 1);
        LaunchFlowInfo launchFlowInfo = createLaunchFlowInfo(processId);
        Launch launch = createLaunch(launchId, launchFlowInfo);
        FlowLaunchEntity flowLaunch = createFlowLaunch(launch, launchFlowInfo);

        LaunchId launchId2 = LaunchId.of(processId, 2);
        LaunchFlowInfo launchFlowInfo2 = createLaunchFlowInfo(processId);
        Launch launch2 = createLaunch(launchId2, launchFlowInfo2);
        FlowLaunchEntity flowLaunch2 = createFlowLaunch(launch2, launchFlowInfo2);

        db.currentOrTx(() -> db.launches().save(launch));
        db.currentOrTx(() -> db.launches().save(launch2));

        db.currentOrTx(() -> {
            launchStateSynchronizer.flowLaunchUpdated(flowLaunch);
            launchStateSynchronizer.flowLaunchUpdated(flowLaunch);
            launchStateSynchronizer.flowLaunchUpdated(flowLaunch2);
        });
    }


    private static LaunchFlowInfo createLaunchFlowInfo(CiProcessId processId) {
        return LaunchFlowInfo.builder()
                .configRevision(TestData.TRUNK_R4)
                .flowId(FlowFullId.of(processId.getPath(), "hotfix"))
                .stageGroupId("my-stages")
                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                .build();
    }

    private static Launch createLaunch(LaunchId launchId, LaunchFlowInfo launchFlowInfo) {
        return Launch.builder()
                .launchId(launchId)
                .title("Мега релиз #42")
                .project("ci")
                .triggeredBy("andreevdm")
                .created(Instant.parse("2020-01-02T10:00:00.000Z"))
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .flowInfo(launchFlowInfo)
                .userData(new LaunchUserData(List.of(), false))
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .status(LaunchState.Status.RUNNING)
                .statusText("")
                .version(Version.major("990"))
                .build();
    }

    private static FlowLaunchEntity createFlowLaunch(Launch launch, LaunchFlowInfo launchFlowInfo) {
        return FlowLaunchEntity.builder()
                .id("first_flow")
                .createdDate(Instant.now())
                .flowInfo(launchFlowInfo)
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .createdDate(Instant.parse("2020-01-02T10:00:00.000Z"))
                .launchId(launch.getLaunchId())
                .launchInfo(LaunchInfo.of(launch.getVersion()))
                .rawStages(
                        new StageGroup("testing", "prestable", "stable")
                                .getStages()
                )
                .projectId("prj")
                .build();
    }

}
