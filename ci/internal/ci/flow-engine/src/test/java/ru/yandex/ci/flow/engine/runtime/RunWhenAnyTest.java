package ru.yandex.ci.flow.engine.runtime;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;
import java.util.List;
import java.util.Queue;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculatorTestBase;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.test_data.common.FailingJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ConvertMultipleRes1ToRes2;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.JobThatShouldProduceRes1ButFailsInstead;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes1;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes1AndFail;
import ru.yandex.ci.flow.test.TestFlowId;

import static ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType.FAILED;
import static ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType.QUEUED;
import static ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType.SUBSCRIBERS_FAILED;
import static ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType.SUCCESSFUL;

public class RunWhenAnyTest extends FlowStateCalculatorTestBase {
    private static final String ZERO_JOB = "zero_job";
    private static final String FIRST_JOB = "first_job";
    private static final String SECOND_JOB = "second_job";
    private static final String THIRD_JOB = "third_job";
    private static final String FIRST_STAGE = "first_stage";
    private static final String SECOND_STAGE = "second_stage";
    private static final String THIRD_STAGE = "third_stage";
    private static final String STAGE_GROUP_ID = "stage_group";

    private StageGroup stageGroup;

    @BeforeEach
    public void setUp() {
        stageGroup = new StageGroup(FIRST_STAGE, SECOND_STAGE, THIRD_STAGE);
        stageGroupSave(StageGroupState.of(STAGE_GROUP_ID));
    }

    @Test
    public void schedulesJobWithOrUpstream() {
        var flowId = flowProvider.register(triangleFlow(false), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowTester.raiseJobExecuteEventsChain(launchId, FIRST_JOB);

        List<TestJobScheduler.TriggeredJob> queuedCommands = new ArrayList<>(testJobScheduler.getTriggeredJobs());

        Assertions.assertEquals(3, queuedCommands.size());
        Assertions.assertEquals(THIRD_JOB, queuedCommands.get(2).getJobLaunchId().getJobId());
    }

    @Test
    public void setsReadyToRun() {
        var flowId = flowProvider.register(triangleFlow(true), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowTester.raiseJobExecuteEventsChain(launchId, FIRST_JOB);

        List<TestJobScheduler.TriggeredJob> queuedCommands = new ArrayList<>(testJobScheduler.getTriggeredJobs());
        FlowLaunchEntity flowLaunch = flowLaunchGet(launchId);
        JobState jobState = flowLaunch.getJobState(THIRD_JOB);

        Assertions.assertEquals(2, queuedCommands.size());
        Assertions.assertTrue(jobState.isReadyToRun());
    }

    @Test
    public void restartsOnlyOnce() {
        FlowLaunchId launchId = flowTester.runFlowToCompletion(triangleFlow(true));

        flowTester.triggerJob(launchId, FIRST_JOB);
        Queue<TestJobScheduler.TriggeredJob> queuedCommands = testJobScheduler.getTriggeredJobs();
        FlowLaunchEntity flowLaunch = flowLaunchGet(launchId);
        JobState jobState = flowLaunch.getJobState(THIRD_JOB);

        Assertions.assertEquals(1, queuedCommands.size());
        Assertions.assertFalse(jobState.isOutdated());
    }

    @Test
    public void producesTwoResourceWhenBothUpstreamsReady() {
        FlowLaunchId launchId = flowTester.runFlowToCompletion(triangleFlow(true));

        flowTester.triggerJob(launchId, THIRD_JOB);
        flowTester.runScheduledJobsToCompletion();

        StoredResourceContainer producedResources = flowTester.getProducedResources(launchId, THIRD_JOB);
        Assertions.assertEquals(2, producedResources.getResources().size());
    }

    @Test
    public void producesOneResourceWhenOneUpstreamReady() {
        FlowLaunchId launchId = flowTester.runFlowToCompletion(triangleFlowWithFailureJob(true));

        flowTester.triggerJob(launchId, THIRD_JOB);
        flowTester.runScheduledJobsToCompletion();

        StoredResourceContainer producedResources = flowTester.getProducedResources(launchId, THIRD_JOB);
        Assertions.assertEquals(1, producedResources.getResources().size());
    }

    @Test
    public void marksAsOutdatedAfterBothUpstreamRestart() {
        FlowLaunchId launchId = flowTester.runFlowToCompletion(triangleFlow(true));

        flowTester.triggerJob(launchId, THIRD_JOB);
        flowTester.runScheduledJobsToCompletion();

        FlowLaunchEntity flowLaunch = flowLaunchGet(launchId);
        JobState jobState = flowLaunch.getJobState(THIRD_JOB);
        Assertions.assertFalse(jobState.isOutdated());

        flowTester.triggerJob(launchId, SECOND_JOB);
        flowTester.triggerJob(launchId, FIRST_JOB);
        flowTester.runScheduledJobsToCompletion();

        flowLaunch = flowLaunchGet(launchId);
        jobState = flowLaunch.getJobState(THIRD_JOB);
        Assertions.assertTrue(jobState.isOutdated());
    }

    static Flow triangleFlow(boolean withManualTrigger) {
        FlowBuilder builder = flowBuilder();

        JobBuilder first = builder.withJob(ProduceRes1.ID, FIRST_JOB);

        JobBuilder second = builder.withJob(ProduceRes1.ID, SECOND_JOB);

        JobBuilder third = builder.withJob(ConvertMultipleRes1ToRes2.ID, THIRD_JOB)
                .withUpstreams(CanRunWhen.ANY_COMPLETED, first, second);

        if (withManualTrigger) {
            third.withManualTrigger();
        }

        return builder.build();
    }

    static Flow triangleFlowWithFailureJob(boolean withManualTrigger) {
        FlowBuilder builder = flowBuilder();

        JobBuilder first = builder.withJob(ProduceRes1.ID, FIRST_JOB);

        JobBuilder second = builder.withJob(JobThatShouldProduceRes1ButFailsInstead.ID, SECOND_JOB);

        JobBuilder third = builder.withJob(ConvertMultipleRes1ToRes2.ID, THIRD_JOB)
                .withUpstreams(CanRunWhen.ANY_COMPLETED, first, second);

        if (withManualTrigger) {
            third.withManualTrigger();
        }

        return builder.build();
    }

    @Test
    public void whenOneOptionalUpstreamWasNotLaunched_LastJobShouldTriggersSuccessfully() {
        FlowBuilder builder = flowBuilder();

        JobBuilder first = builder.withJob(DummyJob.ID, "j1");
        JobBuilder second = builder.withJob(DummyJob.ID, "j2").withManualTrigger();
        JobBuilder third = builder.withJob(ConvertMultipleRes1ToRes2.ID, "j3")
                .withUpstreams(CanRunWhen.ANY_COMPLETED, first, second);

        builder.withJob(ConvertMultipleRes1ToRes2.ID, "last").withUpstreams(third);

        FlowLaunchId launchId = flowTester.runFlowToCompletion(builder.build());
        FlowLaunchEntity flowLaunch = flowLaunchGet(launchId);
        JobState jobState = flowLaunch.getJobState("last");
        Assertions.assertEquals(SUCCESSFUL, jobState.getLastStatusChangeType());
    }

    /**
     * https://github.yandex-team.ru/market-infra/tsum/pull/1794#issuecomment-862258
     */
    @Test
    public void whenOneOptionalUpstreamProducesResourcesAndFails_ItsResourcesShouldNotBePassedToDownstreams() {

        FlowBuilder builder = flowBuilder();

        JobBuilder first = builder.withJob(DummyJob.ID, "j1");
        JobBuilder second = builder.withJob(ProduceRes1AndFail.ID, "j2");

        JobBuilder third = builder.withJob(ConvertMultipleRes1ToRes2.ID, "j3")
                .withUpstreams(CanRunWhen.ANY_COMPLETED, first, second);

        FlowLaunchId launchId = flowTester.runFlowToCompletion(builder.build());
        JobState jobState = flowLaunchGet(launchId).getJobState(third.getId());
        Assertions.assertEquals(SUCCESSFUL, jobState.getLastStatusChangeType());
        Assertions.assertTrue(jobState.getLastLaunch().getConsumedResources().getResources().isEmpty());
    }

    // region Staged flows

    @Test
    public void shouldNotUnlockStage_WhenAnyItsJobIsInProgress() {
        // arrange
        Flow flow = stagedDiamondFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertEquals(SUCCESSFUL, flowLaunch.getJobState(FIRST_JOB).getLastStatusChangeType());
        Assertions.assertEquals(QUEUED, flowLaunch.getJobState(SECOND_JOB).getLastStatusChangeType());
        Assertions.assertEquals(QUEUED, flowLaunch.getJobState(THIRD_JOB).getLastStatusChangeType());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Arrays.asList(FIRST_STAGE, SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldNotUnlockStage_WhenJobInStageIsNotStartedYet() {
        // arrange
        FlowBuilder builder = flowBuilder();
        JobBuilder zeroJob = builder.withJob(DummyJob.ID, ZERO_JOB)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB)
                .withUpstreams(zeroJob);

        JobBuilder secondJob = builder.withJob(DummyJob.ID, SECOND_JOB)
                .withUpstreams(firstJob);

        builder.withJob(DummyJob.ID, THIRD_JOB)
                .withUpstreams(CanRunWhen.ANY_COMPLETED, zeroJob, secondJob)
                .beginStage(stageGroup.getStage(SECOND_STAGE));

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(builder.build(), STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);

        // assert
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Arrays.asList(FIRST_STAGE, SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldNotDisableFlow_IfJobOnFirstStageIsStillRunning() {
        // arrange
        Flow flow = stagedDiamondFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(THIRD_JOB);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertFalse(flowLaunch.isDisabled());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);

        // Здесь не очень очевидный момент, почему не отпускается SECOND_STAGE, ведь на ней все джобы завершились.
        // Дело в том, что последняя стадия не пытается отпуститься при завершении последней джобы в ней,
        // потому что полагается на то что сразу же следом весь флоу будет выкинут из очереди флоу.
        Assertions.assertEquals(
                Arrays.asList(FIRST_STAGE, SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldReleaseSecondStage_WhenJobOnSecondsStageIsFinished_EvenIfJobOnFirstStageIsStillRunning() {

        // arrange
        FlowBuilder builder = stagedDiamondFlowBuilder();

        var upstreams = builder.getJobBuilders().stream()
                .filter(b -> b.getUpstreams().isEmpty())
                .collect(Collectors.toList());

        builder.withJob(DummyJob.ID, "j1").withUpstreams(upstreams).beginStage(stageGroup.getStage(THIRD_STAGE));
        Flow flow = builder.build();

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(THIRD_JOB);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertFalse(flowLaunch.isDisabled());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);

        // Здесь стоит заметить, что в захваченных стадиях появляется "дырка", но это не страшно,
        // SECOND_STAGE всё равно никто не сможет занять, иначе нарушился бы инвариант.
        Assertions.assertEquals(
                Arrays.asList(FIRST_STAGE, THIRD_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldDisableFlow_WhenLastRunningJobOnFirstStageIsFinished() {
        // arrange
        Flow flow = stagedDiamondFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(THIRD_JOB);
        flowTester.runScheduledJobToCompletion(SECOND_JOB);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertTrue(flowLaunch.isDisabled());
        ensureNoFailedJobs(flowLaunch);

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertFalse(stageGroupState.getQueueItem(flowLaunchId).isPresent());
    }

    @Test
    public void shouldReleaseStage_WhenAllItsJobsAreFinished() {
        // arrange
        Flow flow = stagedDiamondFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(SECOND_JOB);

        // assert
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Collections.singletonList(SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldReleaseStageEven_WhenFirstJobFailed_IfItCanPassToNextStage() {
        // arrange
        Flow flow = stagedDiamondFlow(FIRST_JOB);

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(SECOND_JOB);

        // assert
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Collections.singletonList(SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    @Test
    public void shouldReleaseStageEven_WhenSecondJobFailed_IfItCanPassToNextStage() {
        // arrange
        Flow flow = stagedDiamondFlow(SECOND_JOB);

        // act
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowTester.runScheduledJobToCompletion(ZERO_JOB);
        flowTester.runScheduledJobToCompletion(FIRST_JOB);
        flowTester.runScheduledJobToCompletion(SECOND_JOB);

        // assert
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Collections.singletonList(SECOND_STAGE), getAcquiredStageNames(stageGroupState, flowLaunchId)
        );
    }

    private Flow stagedDiamondFlow(String... failingJobIds) {
        return stagedDiamondFlowBuilder(failingJobIds).build();
    }

    private FlowBuilder stagedDiamondFlowBuilder(String... failingJobIds) {
        var failingJobIdSet = Set.of(failingJobIds);
        FlowBuilder builder = flowBuilder();

        Function<String, JobBuilder> jobBuilder = (jobId) -> builder
                .withJob(failingJobIdSet.contains(jobId) ? FailingJob.ID : DummyJob.ID, jobId);

        JobBuilder zeroJob = jobBuilder.apply(ZERO_JOB)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder firstJob = jobBuilder.apply(FIRST_JOB).withUpstreams(zeroJob);
        JobBuilder secondJob = jobBuilder.apply(SECOND_JOB).withUpstreams(zeroJob);

        jobBuilder.apply(THIRD_JOB)
                .withUpstreams(CanRunWhen.ANY_COMPLETED, firstJob, secondJob)
                .beginStage(stageGroup.getStage(SECOND_STAGE));

        return builder;
    }

    private List<String> getAcquiredStageNames(StageGroupState stageGroupState, FlowLaunchId flowLaunchId) {
        return stageGroupState.getAcquiredStages(flowLaunchId).stream()
                .map(StageRef::getId)
                .sorted()
                .collect(Collectors.toList());
    }

    // endregion

    private void ensureNoFailedJobs(FlowLaunchEntity flowLaunch) {
        List<String> failedJobIds = flowLaunch.getJobs().values().stream()
                .filter(j -> EnumSet.of(FAILED, SUBSCRIBERS_FAILED).contains(j.getLastStatusChangeType()))
                .map(JobState::getJobId)
                .collect(Collectors.toList());

        Assertions.assertEquals(Collections.emptyList(), failedJobIds);
    }
}
