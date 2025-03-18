package ru.yandex.ci.engine.launch;

import java.time.Clock;
import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.fasterxml.jackson.core.type.TypeReference;
import org.assertj.core.api.AbstractInstantAssert;
import org.assertj.core.api.ObjectAssert;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockito.junit.jupiter.MockitoSettings;
import org.mockito.quality.Strictness;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.discovery.DiscoveredCommitTable;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.LaunchTable;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchRef;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchRefImpl;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.test.TestFlowId;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.random.TestRandomUtils;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
@MockitoSettings(strictness = Strictness.LENIENT)
class LaunchStateSynchronizerTest {
    private static final long SEED = -1466428502L;
    private static final Instant NOW = Instant.now();

    private Random random;
    private LaunchStateSynchronizer launchStateSynchronizer;
    private AtomicInteger seq;

    @Mock
    private CiMainDb db;
    @Mock
    private LaunchTable launchTable;
    @Mock
    private DiscoveredCommitTable discoveredCommitTable;
    @Mock
    private PullRequestService pullRequestService;
    @Mock
    private BranchService branchService;
    @Mock
    private EventPublisher eventPublisher;
    @Mock
    private CommitRangeService commitRangeService;
    @Mock
    private BazingaTaskManager bazingaTaskManager;
    @Mock
    private XivaNotifier xivaNotifier;

    private final Clock clock = Clock.systemUTC();

    @BeforeEach
    void setUp() {
        random = new Random(SEED);
        seq = new AtomicInteger();
        launchStateSynchronizer = new LaunchStateSynchronizer(db, pullRequestService, branchService,
                commitRangeService, eventPublisher, bazingaTaskManager, clock, xivaNotifier);
        mockDatabase();
    }

    @Test
    void dontUpdateAlreadyFinished() {
        FlowLaunchEntity flowLaunch = flowLaunch(idle());
        Launch launch = launchOf(flowLaunch, Status.SUCCESS);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        verify(launchTable, never()).save(any(Launch.class));
    }

    @Test
    void runningToFinished() {
        FlowLaunchEntity flowLaunch = flowLaunch(idle(), jobState(StatusChange.successful()));
        Launch launch = launchOf(flowLaunch, Status.RUNNING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch)
                .hasStatus(Status.SUCCESS)
                .satisfies(updated ->
                        assertThat(updated.getFinished()).isNotNull());
    }

    @Test
    void treatIdleAfterCancellingAsCancelled() {
        FlowLaunchEntity flowLaunch = flowLaunch(idle()).withDisabled(true);

        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch)
                .hasStatus(Status.CANCELED)
                .satisfies(updated ->
                        assertThat(updated.getFinished()).isNotNull());
    }

    @Test
    void disabledFlowBufNotCancellingIsFinished() {
        FlowLaunchEntity flowLaunch = flowLaunch(idle()).withDisabled(true);
        Launch launch = launchOf(flowLaunch, Status.RUNNING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).hasStatus(Status.SUCCESS);
    }

    @Test
    void dontResetCancellingState() {
        FlowLaunchEntity flowLaunch = flowLaunch(runningWithErrors(), jobState(StatusChange.failed()));
        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        verify(launchTable, never()).save(any(Launch.class));
    }

    @Test
    void treatFailureAsCancelling() {
        FlowLaunchEntity flowLaunch = flowLaunch(failure(), jobState(StatusChange.failed()));
        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        verify(launchTable, never()).save(any(Launch.class));
    }

    @Test
    void treatFailureAsCancelledIfJobDisabled() {
        JobState jobState = jobState(StatusChange.failed());
        jobState.setDisabled(true);
        FlowLaunchEntity flowLaunch = flowLaunch(failure(), jobState).withDisabled(true);

        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).hasStatus(Status.CANCELED);
    }

    @Test
    public void failureUpdatesFinishedDateForActions() {
        JobState jobState = jobState(StatusChange.failed());
        FlowLaunchEntity flowLaunch = customFlowLaunch(failure(), FlowTestUtils.ACTION_LAUNCH_ID, jobState);
        Launch launch = launchOf(flowLaunch, Status.RUNNING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).finished().isNotNull();
    }

    @Test
    public void failureLeavesNullFinishedDateForReleases() {
        JobState jobState = jobState(StatusChange.failed());
        FlowLaunchEntity flowLaunch = flowLaunch(failure(), jobState);
        Launch launch = launchOf(flowLaunch, Status.RUNNING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).finished().isNull();
    }

    @Test
    void treatFailureAsCancelled_ifFlowLaunchDisabled() {
        FlowLaunchEntity flowLaunch = flowLaunch(failure()).withDisabled(true);
        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).hasStatus(Status.CANCELED);
    }

    @Test
    void registerInDiscoveredCommit() {
        FlowLaunchEntity flowLaunch = flowLaunch(failure()).withDisabled(true);
        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        @SuppressWarnings("unchecked")
        ArgumentCaptor<Function<Optional<DiscoveredCommitState>, DiscoveredCommitState>> functionArgumentCaptor =
                ArgumentCaptor.forClass(Function.class);

        verify(discoveredCommitTable).updateOrCreate(
                eq(launch.getLaunchId().getProcessId()),
                eq(launch.getVcsInfo().getRevision()),
                functionArgumentCaptor.capture()
        );

        DiscoveredCommitState updated = functionArgumentCaptor.getValue()
                .apply(Optional.of(DiscoveredCommitState.builder().build()));

        assertThat(updated).isNotNull();
        assertThat(updated.getCancelledLaunchIds()).containsExactly(launch.getLaunchId());
        assertThat(updated.getDisplacedLaunchIds()).isEmpty();
    }

    @Test
    void registerInDiscoveredCommitWithDisplacement() {
        FlowLaunchEntity flowLaunch = flowLaunch(failure()).withDisabled(true);
        Launch launch = launchOf(flowLaunch, Status.CANCELLING).toBuilder()
                .displaced(true)
                .build();
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        @SuppressWarnings("unchecked")
        ArgumentCaptor<Function<Optional<DiscoveredCommitState>, DiscoveredCommitState>> functionArgumentCaptor =
                ArgumentCaptor.forClass(Function.class);

        verify(discoveredCommitTable).updateOrCreate(
                eq(launch.getLaunchId().getProcessId()),
                eq(launch.getVcsInfo().getRevision()),
                functionArgumentCaptor.capture()
        );

        DiscoveredCommitState updated = functionArgumentCaptor.getValue()
                .apply(Optional.of(DiscoveredCommitState.builder().build()));

        assertThat(updated).isNotNull();
        assertThat(updated.getCancelledLaunchIds()).containsExactly(launch.getLaunchId());
        assertThat(updated.getDisplacedLaunchIds()).containsExactly(launch.getLaunchId());
    }

    @Test
    void dontAddToCancelledIfNotCancelledActually() {
        FlowLaunchEntity flowLaunch = flowLaunch(idle()).withDisabled(true);
        Launch launch = launchOf(flowLaunch, Status.RUNNING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).hasStatus(Status.SUCCESS);

        //noinspection unchecked
        verify(discoveredCommitTable, never()).updateOrCreate(any(), any(), any(Function.class));
    }

    @Test
    void disabledFlowWithWaitForManualTriggerIsCancelled() {
        var jobs = TestUtils.parseJson("flow-jobs/waiting-manual-trigger.json",
                new TypeReference<Map<String, JobState>>() {
                });
        JobState[] jobStates = jobs.values().toArray(JobState[]::new);
        FlowLaunchEntity flowLaunch = staged(flowLaunch(idle(), jobStates))
                .withDisabled(true);

        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);

        assertThatUpdated(launch).hasStatus(Status.CANCELED);
    }

    @Test
    void notDisabledFlowYetWithWaitForManualTriggerIsCancelled() {
        var jobs = TestUtils.parseJson("flow-jobs/waiting-manual-trigger.json",
                new TypeReference<Map<String, JobState>>() {
                });
        FlowLaunchEntity flowLaunch = staged(flowLaunch(idle(), jobs.values().toArray(JobState[]::new)))
                .withDisabled(false);

        Launch launch = launchOf(flowLaunch, Status.CANCELLING);
        when(launchTable.get(flowLaunch.getLaunchId())).thenReturn(launch);

        launchStateSynchronizer.flowLaunchUpdated(flowLaunch);
        verify(launchTable, never()).save(any(Launch.class));
    }

    private ru.yandex.ci.flow.engine.runtime.state.model.LaunchState idle() {
        return ru.yandex.ci.flow.engine.runtime.state.model.LaunchState.IDLE;
    }

    private ru.yandex.ci.flow.engine.runtime.state.model.LaunchState runningWithErrors() {
        return ru.yandex.ci.flow.engine.runtime.state.model.LaunchState.RUNNING_WITH_ERRORS;
    }

    private ru.yandex.ci.flow.engine.runtime.state.model.LaunchState failure() {
        return ru.yandex.ci.flow.engine.runtime.state.model.LaunchState.FAILURE;
    }

    private Launch launchOf(FlowLaunchEntity flowLaunch, Status status) {
        return Launch.builder()
                .launchId(flowLaunch.getLaunchId())
                .status(status)
                .statusText("")
                .notifyPullRequest(false)
                .flowLaunchId(flowLaunch.getFlowLaunchId().asString())
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(OrderedArcRevision.fromHash("b0", "trunk", 1, 0))
                                .build()
                )
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(TestData.TRUNK_R1)
                        .flowId(flowLaunch.getFlowFullId())
                        .stageGroupId("my-stages")
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                        .build())
                .started(NOW)
                .version(Version.major("1"))
                .build();
    }

    private static FlowLaunchEntity staged(FlowLaunchEntity flowLaunch) {
        Stage stage = StageBuilder
                .create("first_stage")
                .withStageGroup(new StageGroup("first_stage"))
                .build();

        return flowLaunch.toBuilder().rawStages(List.of(stage))
                .flowInfo(FlowTestUtils.toFlowInfo(flowLaunch.getFlowFullId(), "stageGroupId"))
                .build();

    }

    private static FlowLaunchEntity flowLaunch(ru.yandex.ci.flow.engine.runtime.state.model.LaunchState state,
                                               JobState... jobStates) {
        return customFlowLaunch(state, FlowTestUtils.LAUNCH_ID, jobStates);
    }

    private static FlowLaunchEntity customFlowLaunch(LaunchState state, LaunchId launchId, JobState... jobStates) {
        FlowLaunchRef launchRef = FlowLaunchRefImpl.create(FlowLaunchId.of("flow-launch-1"), TestFlowId.of("flow-id"));

        return FlowLaunchEntity.builder()
                .id(launchRef.getFlowLaunchId())
                .createdDate(NOW)
                .flowInfo(FlowTestUtils.toFlowInfo(launchRef.getFlowFullId(), null))
                .jobs(Stream.of(jobStates).collect(Collectors.toMap(
                        JobState::getJobId,
                        Function.identity()
                )))
                .launchId(launchId)
                .launchInfo(LaunchInfo.of(String.valueOf(FlowTestUtils.LAUNCH_ID.getNumber())))
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .projectId("prj")
                .state(state)
                .build();
    }

    private JobState jobState(StatusChange... statusHistory) {
        String id = TestRandomUtils.string(random, "job-id");

        Job job = mock(Job.class);
        var executor = ExecutorContext.internal(InternalExecutorContext.of(DummyJob.ID));
        when(job.getExecutorContext()).thenReturn(executor);
        when(job.getId()).thenReturn(id);
        JobState jobState = new JobState(
                job,
                Collections.emptySet(),
                ResourceRefContainer.empty(),
                null
        );
        jobState.addLaunch(new JobLaunch(seq.getAndIncrement(), "user-triggered", List.of(), List.of(statusHistory)));
        return jobState;
    }

    private void mockDatabase() {
        TestCiDbUtils.mockToCallRealTxMethods(db);
        doReturn(launchTable).when(db).launches();
        doReturn(discoveredCommitTable).when(db).discoveredCommit();
    }

    public LaunchAssert assertThatUpdated(Launch launch) {
        ArgumentCaptor<Launch> savedLaunch = ArgumentCaptor.forClass(Launch.class);
        verify(launchTable).save(savedLaunch.capture());

        return new LaunchAssert(savedLaunch.getValue())
                .hasSameIdAs(launch);
    }

    private static class LaunchAssert extends ObjectAssert<Launch> {

        LaunchAssert(Launch launch) {
            super(launch);
        }

        @SuppressWarnings("ReferenceEquality")
        public LaunchAssert hasSameIdAs(Launch launch) {
            isNotNull();
            assertThat(launch).isNotNull();

            if (actual.getId() != launch.getId()) {
                failWithMessage(
                        "Expected launch %s to have id <%s> but was <%s>",
                        actual,
                        launch.getId(),
                        actual.getId()
                );
            }

            return this;
        }

        public LaunchAssert hasStatus(Status status) {
            isNotNull();
            assertThat(actual.getStatus()).isNotNull();

            var actualStatus = actual.getStatus();
            if (actualStatus != status) {
                failWithMessage(
                        "Expected %s to have status <%s> but was <%s>",
                        actual,
                        status,
                        actualStatus
                );
            }

            return this;
        }

        public AbstractInstantAssert<?> finished() {
            return assertThat(actual.getFinished());
        }

    }
}
