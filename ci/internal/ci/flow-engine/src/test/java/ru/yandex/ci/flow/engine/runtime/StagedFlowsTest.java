package ru.yandex.ci.flow.engine.runtime;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;

import com.google.common.collect.Sets;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.common.FailingJob;
import ru.yandex.ci.flow.test.TestFlowId;

import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class StagedFlowsTest extends FlowEngineTestBase {
    private static final String STAGE_GROUP_ID = "test-stages";
    private static final String FIRST_JOB_ID = "FIRST_JOB";
    private static final String SECOND_JOB_ID = "SECOND_JOB";
    private static final String THIRD_JOB_ID = "THIRD_JOB";

    private static final String FIRST_STAGE = "first";
    private static final String SECOND_STAGE = "second";

    private final StageGroup stageGroup = new StageGroup(FIRST_STAGE, SECOND_STAGE);
    private static final String MIDDLE_JOB_ID = "MIDDLE_JOB";

    @Test
    public void failedJobShouldNotReleaseStage() {
        // arrange
        FlowBuilder builder = flowBuilder();

        builder.withJob(FailingJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        Flow flow = builder.build();

        // act
        FlowLaunchId launchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);

        // assert
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Sets.newHashSet(FIRST_STAGE),
                getAcquiredStageIds(launchId, stageGroupState)
        );
    }

    @Test
    public void stageShouldNotBeReleasedUntilNextJobIsTriggered() {
        Flow flow = simpleTwoStageFlow();
        FlowLaunchId launchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Sets.newHashSet(FIRST_STAGE),
                getAcquiredStageIds(launchId, stageGroupState)
        );

        Assertions.assertTrue(flowLaunchGet(launchId).getJobState(SECOND_JOB_ID).isReadyToRun());
    }

    @Test
    public void secondFlowWaitsForLockedStage() {
        Flow flow = simpleTwoStageFlow();
        FlowLaunchId firstLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);
        FlowLaunchId secondLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);

        FlowLaunchEntity firstLaunch = flowLaunchGet(firstLaunchId);
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                firstLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertNull(firstLaunch.getJobState(SECOND_JOB_ID).getLastLaunch());
        Assertions.assertTrue(firstLaunch.getJobState(SECOND_JOB_ID).isReadyToRun());

        FlowLaunchEntity secondLaunch = flowLaunchGet(secondLaunchId);
        Assertions.assertEquals(
                StatusChangeType.WAITING_FOR_STAGE,
                secondLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertFalse(secondLaunch.getJobState(FIRST_JOB_ID).isReadyToRun());
    }

    @Test
    public void manyFlowsWaitingForFirstStage() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(firstJob)
                .withManualTrigger();

        Flow flow = builder.build();

        FlowLaunchId firstLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);
        FlowLaunchId secondLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);
        FlowLaunchId thirdLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);
        FlowLaunchId fourthLaunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(
                Sets.newHashSet(FIRST_STAGE),
                getAcquiredStageIds(firstLaunchId, stageGroupState)
        );

        Assertions.assertEquals(
                Collections.emptySet(),
                getAcquiredStageIds(secondLaunchId, stageGroupState)
        );

        Assertions.assertEquals(
                Collections.emptySet(),
                getAcquiredStageIds(thirdLaunchId, stageGroupState)
        );

        Assertions.assertEquals(
                Collections.emptySet(),
                getAcquiredStageIds(fourthLaunchId, stageGroupState)
        );
    }

    private Set<String> getAcquiredStageIds(FlowLaunchId firstLaunchId, StageGroupState stageGroupState) {
        return getAcquiredStages(firstLaunchId, stageGroupState);
    }

    @Test
    public void twoFlowsFullCycle() {
        Flow flow = simpleTwoStageFlow();
        FlowLaunchId firstLaunchId = flowTester.runFlowToCompletion(flow);
        FlowLaunchId secondLaunchId = flowTester.runFlowToCompletion(flow);

        // act
        flowTester.triggerJob(firstLaunchId, SECOND_JOB_ID);
        flowTester.runScheduledJobsToCompletion();

        // assert first flow finished, second waits SECOND_JOB_ID trigger
        FlowLaunchEntity firstLaunch = flowLaunchGet(firstLaunchId);
        FlowLaunchEntity secondLaunch = flowLaunchGet(secondLaunchId);
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                firstLaunch.getJobState(SECOND_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertNull(secondLaunch.getJobState(SECOND_JOB_ID).getLastLaunch());
        Assertions.assertEquals(
                0,
                secondLaunch.getStatistics().getRunning()
        );
        Assertions.assertEquals(
                LaunchState.WAITING_FOR_MANUAL_TRIGGER,
                secondLaunch.getState()
        );

        // act
        flowTester.triggerJob(secondLaunchId, SECOND_JOB_ID);

        secondLaunch = flowLaunchGet(secondLaunchId);
        Assertions.assertEquals(
                1,
                secondLaunch.getStatistics().getRunning()
        );
        Assertions.assertEquals(
                LaunchState.RUNNING,
                secondLaunch.getState()
        );

        flowTester.runScheduledJobsToCompletion();

        // assert both flows finished
        secondLaunch = flowLaunchGet(secondLaunchId);
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                secondLaunch.getJobState(SECOND_JOB_ID).getLastStatusChangeType()
        );
    }

    @Test
    public void cannotTriggerMiddleJobInPastStage() {
        // arrange
        Flow flow = longTwoStageFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow);

        // assert
        FlowLaunchEntity flowLaunch = flowLaunchGet(flowLaunchId);
        Assertions.assertFalse(flowLaunch.getJobState(MIDDLE_JOB_ID).isReadyToRun());
    }

    @Test
    // это тест на случай, когда механизм rollback'а ещё не готов
    public void cannotTriggerAnyJobInPasteStage() {
        // arrange
        Flow flow = longTwoStageFlow();

        // act
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow);

        // assert
        FlowLaunchEntity flowLaunch = flowLaunchGet(flowLaunchId);
        Assertions.assertFalse(flowLaunch.getJobState(FIRST_JOB_ID).isReadyToRun());
    }

    @Test
    public void unlocksOldStagesWhenAcquiringNewStage() {
        var flowId = flowTester.register(secondStageWithAnyLinkFlow(), TestFlowId.TEST_PATH);

        FlowLaunchId firstFlowLaunchId = flowTester.tryRunFlowToCompletion(flowId, STAGE_GROUP_ID);
        FlowLaunchId secondFlowLaunchId = flowTester.tryRunFlowToCompletion(flowId, STAGE_GROUP_ID);
        FlowLaunchEntity firstFlowLaunch = flowLaunchGet(firstFlowLaunchId);
        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);

        Assertions.assertEquals(
                StatusChangeType.FAILED,
                firstFlowLaunch.getJobState(THIRD_JOB_ID).getLastStatusChangeType()
        );

        Assertions.assertEquals(
                Collections.singleton(SECOND_STAGE),
                getAcquiredStages(firstFlowLaunchId, stageGroupState)
        );
        Assertions.assertEquals(
                Collections.singleton(FIRST_STAGE),
                getAcquiredStages(secondFlowLaunchId, stageGroupState)
        );

        flowTester.triggerJob(firstFlowLaunchId, THIRD_JOB_ID);
        flowTester.runScheduledJobsToCompletion();
        stageGroupState = Objects.requireNonNull(stageGroupGet(STAGE_GROUP_ID));

        Assertions.assertEquals(
                Collections.singleton(SECOND_STAGE),
                getAcquiredStages(secondFlowLaunchId, stageGroupState)
        );
    }

    private Set<String> getAcquiredStages(FlowLaunchId secondFlowLaunchId, StageGroupState stageGroupState) {
        return stageGroupState.getAcquiredStages(secondFlowLaunchId).stream()
                .map(StageRef::getId)
                .collect(Collectors.toSet());
    }

    @Test
    public void doesNotUnlockOldStagesWhenAcquiringNewStageAndRunningJobsOnOldStages() {
        var flow = flowTester.register(oneFirstStageJobTwoSecondWithManualEnd(), TestFlowId.TEST_PATH);

        FlowLaunchId firstFlowLaunchId = flowTester.tryRunFlowToCompletion(flow, STAGE_GROUP_ID);
        FlowLaunchId secondFlowLaunchId = flowTester.tryRunFlowToCompletion(flow, STAGE_GROUP_ID);

        flowTester.triggerJob(secondFlowLaunchId, FIRST_JOB_ID);
        flowTester.triggerJob(firstFlowLaunchId, THIRD_JOB_ID);
        flowTester.runScheduledJobToCompletion(THIRD_JOB_ID);

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);

        Assertions.assertEquals(
                Collections.emptySet(),
                getAcquiredStages(firstFlowLaunchId, stageGroupState)
        );

        Assertions.assertEquals(
                new HashSet<>(Arrays.asList(FIRST_STAGE, SECOND_STAGE)),
                getAcquiredStages(secondFlowLaunchId, stageGroupState)
        );
    }

    public Flow secondStageWithAnyLinkFlow() {
        FlowBuilder builder = flowBuilder();

        JobBuilder startingStageJob = builder.withJob(DummyJob.ID, "j1")
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder firstParallelJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .withUpstreams(startingStageJob)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder secondParallelJob = builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(startingStageJob);

        builder.withJob(OnceFailingJob.ID, THIRD_JOB_ID)
                .withUpstreams(CanRunWhen.ANY_COMPLETED, firstParallelJob, secondParallelJob)
                .beginStage(stageGroup.getStage(SECOND_STAGE));

        return builder.build();
    }

    @Test
    public void scheduleAutoReleaseWhenUnlockingStage() {
        Flow flow = simpleTwoStageFlow();
        FlowLaunchId firstLaunchId = flowTester.runFlowToCompletion(flow);

        flowTester.triggerJob(firstLaunchId, SECOND_JOB_ID);
        flowTester.runScheduledJobsToCompletion();

        Assertions.assertEquals(
                Set.of(firstLaunchId),
                flowTester.getFlowsWhichTriggeredAutoReleases()
        );
    }

    @Test
    public void multipleStageAcquire() {
        Flow flow = flowWithTwoFirstStage();
        assertThatThrownBy(() -> flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID))
                .hasMessageContaining("Flow already acquired stage");
    }

    // TODO: тест отката флоу
    // TODO: кейс, когда один флоу дизаблится, а другой должен захватить его секцию

    private Flow simpleTwoStageFlow() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(firstJob)
                .withManualTrigger()
                .beginStage(stageGroup.getStage(SECOND_STAGE));

        return builder.build();
    }

    private Flow longTwoStageFlow() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder middleJob = builder.withJob(DummyJob.ID, MIDDLE_JOB_ID)
                .withUpstreams(firstJob);

        builder.withJob(DummyJob.ID, "LAST_JOB")
                .beginStage(stageGroup.getStage(SECOND_STAGE))
                .withUpstreams(middleJob);

        return builder.build();
    }

    private Flow oneFirstStageJobTwoSecondWithManualEnd() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder middleJob = builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .beginStage(stageGroup.getStage(SECOND_STAGE))
                .withUpstreams(firstJob);

        builder.withJob(DummyJob.ID, THIRD_JOB_ID)
                .withManualTrigger()
                .withUpstreams(middleJob);

        return builder.build();
    }

    private Flow flowWithTwoFirstStage() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        JobBuilder secondJob = builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));


        builder.withJob(DummyJob.ID, "LAST_JOB")
                .beginStage(stageGroup.getStage(SECOND_STAGE))
                .withUpstreams(firstJob, secondJob);

        return builder.build();
    }

    public static class OnceFailingJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("fbfff8b0-cf9a-4a5c-bae7-ecc36189c180");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            if (context.getJobState().getLastLaunch().getNumber() == 1) {
                throw new RuntimeException("I am once failing job!");
            }
        }
    }
}
