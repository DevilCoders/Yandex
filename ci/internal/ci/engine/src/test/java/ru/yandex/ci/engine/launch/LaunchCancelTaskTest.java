package ru.yandex.ci.engine.launch;

import java.time.Clock;
import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.commune.bazinga.BazingaTryLaterException;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

class LaunchCancelTaskTest extends YdbCiTestBase {

    private static final LaunchId LAUNCH_ID = LaunchId.of(TestData.SIMPLE_RELEASE_PROCESS_ID, 1);

    @MockBean
    private FlowLaunchService flowLaunchService;
    @MockBean
    private LaunchStateSynchronizer launchStateSynchronizer;
    @MockBean
    private FlowLaunchMutexManager flowLaunchMutexManager;

    private LaunchCancelTask launchCancelTask;

    @Mock
    private ExecutionContext context;
    private final Clock clock = Clock.systemUTC();

    @BeforeEach
    void setUp() {
        doAnswer(invocation -> {
            var runnable = (Runnable) invocation.getArgument(1);
            runnable.run();
            return null;
        }).when(flowLaunchMutexManager).acquireAndRun(any(LaunchId.class), any(Runnable.class));

        launchCancelTask = new LaunchCancelTask(db, flowLaunchService, launchStateSynchronizer, clock,
                flowLaunchMutexManager);
    }

    @Test
    void cancelAlreadyStarted() {
        db.currentOrTx(() -> db.launches().save(
                launchBuilder(LAUNCH_ID, LaunchState.Status.CANCELLING)
                        .flowLaunchId(FlowLaunchId.of(LAUNCH_ID).asString())
                        .build())
        );

        launchCancelTask.execute(new LaunchCancelTask.Params(LAUNCH_ID), context);

        verify(flowLaunchService).cancelFlow(FlowLaunchId.of(LAUNCH_ID));
        verify(launchStateSynchronizer, never()).stateUpdated(any(), any(), anyBoolean(), any());
    }

    @Test
    void cancelAlreadyFinished() {
        db.currentOrTx(() -> db.launches().save(
                launchBuilder(LAUNCH_ID, LaunchState.Status.SUCCESS)
                        .build())
        );

        launchCancelTask.execute(new LaunchCancelTask.Params(LAUNCH_ID), context);

        verify(flowLaunchService, never()).cancelFlow(any());
        verify(launchStateSynchronizer, never()).stateUpdated(any(), any(), anyBoolean(), any());
    }

    @Test
    void cancelFlowNotStartedYet() {
        db.currentOrTx(() -> db.launches().save(
                launchBuilder(LAUNCH_ID, LaunchState.Status.CANCELLING)
                        .flowLaunchId(null)
                        .build())
        );

        launchCancelTask.execute(new LaunchCancelTask.Params(LAUNCH_ID), context);

        verify(flowLaunchService, never()).cancelFlow(any());
        ArgumentCaptor<LaunchState> stateCaptor = ArgumentCaptor.forClass(LaunchState.class);
        verify(launchStateSynchronizer).stateUpdated(
                argThat(launch -> LAUNCH_ID.equals(launch.getLaunchId())), stateCaptor.capture(), anyBoolean(), any());

        assertThat(stateCaptor.getValue().getFinished()).isNotEmpty();
        assertThat(stateCaptor.getValue().getStatus()).isEqualTo(LaunchState.Status.CANCELED);
    }

    @Test
    void cancelNotMarkedCancelledYet() {
        db.currentOrTx(() -> db.launches().save(
                launchBuilder(LAUNCH_ID, LaunchState.Status.RUNNING)
                        .build())
        );

        assertThatThrownBy(() -> launchCancelTask.execute(new LaunchCancelTask.Params(LAUNCH_ID), context))
                .isInstanceOf(BazingaTryLaterException.class);
    }

    private static Launch.Builder launchBuilder(LaunchId launchId, LaunchState.Status status) {
        return Launch.builder()
                .launchId(launchId)
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(TestData.TRUNK_R4)
                        .flowId(FlowFullId.of(LAUNCH_ID.getProcessId().getPath(), LAUNCH_ID.getProcessId().getSubId()))
                        .stageGroupId("my-stages")
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                        .build())
                .vcsInfo(LaunchVcsInfo.builder()
                        .revision(OrderedArcRevision.fromHash("b2", "trunk", 42, 0))
                        .previousRevision(OrderedArcRevision.fromHash("b1", "trunk", 21, 0))
                        .pullRequestInfo(
                                new LaunchPullRequestInfo(
                                        1L,
                                        1L,
                                        TestData.CI_USER,
                                        null,
                                        null,
                                        ArcanumMergeRequirementId.of("system", "type"),
                                        new PullRequestVcsInfo(
                                                TestData.REVISION,
                                                TestData.SECOND_REVISION,
                                                ArcBranch.trunk(),
                                                TestData.THIRD_REVISION,
                                                TestData.USER_BRANCH
                                        ),
                                        List.of("CI-1"),
                                        List.of("label1"),
                                        null
                                )
                        )
                        .commitCount(7)
                        .build()
                )
                .version(Version.major("99"))
                .status(status);
    }
}

