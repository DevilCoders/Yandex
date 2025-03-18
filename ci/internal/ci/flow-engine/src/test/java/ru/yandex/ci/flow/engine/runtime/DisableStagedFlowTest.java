package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.assertj.core.api.Assertions.assertThat;

public class DisableStagedFlowTest extends FlowEngineTestBase {
    private static final String FIRST_JOB_ID = "FIRST_JOB";
    private static final String SECOND_JOB_ID = "SECOND_JOB";
    private static final String THIRD_JOB_ID = "THIRD_JOB";
    private static final String STAGE_GROUP_ID = "STAGE_GROUP_ID";

    @Autowired
    private FlowStateService flowStateService;

    private StageGroup stageGroup;

    @BeforeEach
    public void setUp() {
        stageGroup = new StageGroup("first_stage", "second_stage");
    }

    @Test
    public void shouldDisableFinishedFlowAndRemoveItFromQueue() {
        // arrange
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow(), STAGE_GROUP_ID);

        // act
        flowTester.triggerJob(flowLaunchId, SECOND_JOB_ID);
        flowTester.runScheduledJobsToCompletion();

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertTrue(flowLaunch.isDisabled());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        assertThat(stageGroupState.getQueue()).isEmpty();

        assertThat(flowTester.getFlowsWhichTriggeredAutoReleases()).containsExactly(flowLaunchId);
    }

    @Test
    public void shouldDisableFinishedFlow_WithSeveralLastJobs() {
        // arrange
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStages().get(0));

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(firstJob);

        builder.withJob(DummyJob.ID, THIRD_JOB_ID)
                .withUpstreams(firstJob);

        // act
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(builder.build(), STAGE_GROUP_ID);
        flowTester.runScheduledJobsToCompletion();

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertTrue(flowLaunch.isDisabled());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        assertThat(stageGroupState.getQueue()).isEmpty();
        assertThat(flowTester.getFlowsWhichTriggeredAutoReleases()).containsExactly(flowLaunchId);
    }

    @Test
    public void shouldDisableFlowIfItHasWaitingForStageJobs() {
        // arrange
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStages().get(0));

        JobBuilder secondJob = builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(firstJob)
                .beginStage(stageGroup.getStages().get(1));

        builder.withJob(DummyJob.ID, "dummy")
                .withManualTrigger()
                .withUpstreams(secondJob);

        Flow twoStageFlow = builder.build();

        // act
        FlowLaunchId firstLaunchId = flowTester.runFlowToCompletion(twoStageFlow, STAGE_GROUP_ID);
        assertThat(flowTester.getFlowsWhichTriggeredAutoReleases()).containsExactly(firstLaunchId);
        flowTester.getFlowsWhichTriggeredAutoReleases().clear();

        FlowLaunchId secondLaunchId = flowTester.runFlowToCompletion(twoStageFlow, STAGE_GROUP_ID);
        flowStateService.disableLaunchGracefully(secondLaunchId, true);

        // assert
        FlowLaunchEntity secondFlowLaunch = flowTester.getFlowLaunch(secondLaunchId);
        Assertions.assertTrue(secondFlowLaunch.isDisabled());
        Assertions.assertEquals(
                StatusChangeType.WAITING_FOR_STAGE,
                secondFlowLaunch.getJobState(SECOND_JOB_ID).getLastStatusChangeType()
        );

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        Assertions.assertEquals(1, stageGroupState.getQueue().size());
        assertThat(flowTester.getFlowsWhichTriggeredAutoReleases()).containsExactly(secondLaunchId);
    }

    @Test
    public void shouldRemoveFromQueueWhenDisabledManually() {
        // arrange
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow(), STAGE_GROUP_ID);

        // act
        flowStateService.disableLaunchGracefully(flowLaunchId, true);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        Assertions.assertTrue(flowLaunch.isDisabled());

        StageGroupState stageGroupState = stageGroupGet(STAGE_GROUP_ID);
        assertThat(stageGroupState.getQueue()).isEmpty();
        assertThat(flowTester.getFlowsWhichTriggeredAutoReleases()).containsExactly(flowLaunchId);
    }

    private Flow flow() {
        FlowBuilder builder = flowBuilder();

        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStages().get(0));

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withManualTrigger()
                .withUpstreams(firstJob);

        return builder.build();
    }
}
