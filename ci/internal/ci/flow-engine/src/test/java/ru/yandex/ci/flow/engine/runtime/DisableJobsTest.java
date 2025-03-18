package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorFailedToInterruptEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobDisabledException;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

public class DisableJobsTest extends FlowEngineTestBase {
    private static final String FIRST_JOB_ID = "FIRST_JOB";
    private static final String SECOND_JOB_ID = "SECOND_JOB";
    private static final String STAGE_ID = "STAGE_ID";
    public static final String STAGE_GROUP_ID = "stageGroupId";

    @Autowired
    private FlowStateService flowStateService;

    @Test
    public void manualTriggersDontWorkOnDisabledJobs() {
        Assertions.assertThrows(JobDisabledException.class, () -> {
            // arrange
            FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleFlow.FLOW_ID);
            flowStateService.disableJobsInLaunchGracefully(flowLaunchId, jobState -> true, false, false);

            // act
            flowStateService.recalc(flowLaunchId, new TriggerEvent(SimpleFlow.JOB_ID, "user42", false));
        });
    }

    @Test
    public void disableJobsByPredicate() {
        // arrange
        FlowBuilder builder = flowBuilder();
        Flow flow = builder.build();

        // act
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow);
        flowStateService.disableJobsInLaunchGracefully(
                flowLaunchId,
                jobState -> FIRST_JOB_ID.equals(jobState.getJobId()),
                false,
                false
        );

        // assert
        FlowLaunchEntity flowLaunch = flowLaunchGet(flowLaunchId);
        assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        assertFalse(flowLaunch.getJobState(SECOND_JOB_ID).isDisabled());
    }

    @Test
    public void disableJobsGracefully() {
        // arrange
        FlowBuilder builder = flowBuilder();
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow);
        FlowLaunchEntity launch = flowStateService.recalc(
                launchId,
                new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create())
        );

        flowLaunchSave(launch);

        // act
        flowStateService.disableJobsInLaunchGracefully(launchId, jobState -> true, false, false);
        flowStateService.recalc(launchId, new ExecutorFailedToInterruptEvent(FIRST_JOB_ID, 1));

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabling());
        Assertions.assertFalse(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertTrue(flowLaunch.getJobState(SECOND_JOB_ID).isDisabled());

        // act
        flowTester.runScheduledJobsToCompletion();

        // assert
        flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                flowLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertNull(flowLaunch.getJobState(SECOND_JOB_ID).getLastLaunch());
    }

    @Test
    public void disableJobsInStagedFlowGracefully() {
        // arrange
        StageGroup stageGroup = new StageGroup(STAGE_ID);

        FlowBuilder builder = flowBuilder();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStage(STAGE_ID));
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_ID);
        flowStateService.recalc(
                launchId,
                new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create())
        );

        // act
        flowStateService.disableJobsInLaunchGracefully(launchId, jobState -> true, false, false);
        flowStateService.recalc(launchId, new ExecutorFailedToInterruptEvent(FIRST_JOB_ID, 1));

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabling());
        Assertions.assertFalse(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertTrue(flowLaunch.getJobState(SECOND_JOB_ID).isDisabled());

        // act
        flowTester.runScheduledJobsToCompletion();

        // assert
        flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                flowLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertNull(flowLaunch.getJobState(SECOND_JOB_ID).getLastLaunch());
    }

    @Test
    public void disableJobsOnUninterruptibleStage() {
        // arrange
        StageGroup stageGroup = new StageGroup(StageBuilder.create(STAGE_ID).uninterruptable());

        FlowBuilder builder = flowBuilder();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStage(STAGE_ID));
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_ID);
        flowStateService.recalc(
                launchId,
                new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create())
        );

        // act
        flowStateService.disableJobsInLaunchGracefully(launchId, jobState -> true, false, false);
        flowTester.runScheduledJobsToCompletion();

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertTrue(flowLaunch.getJobState(SECOND_JOB_ID).isDisabled());
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                flowLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                flowLaunch.getJobState(SECOND_JOB_ID).getLastStatusChangeType()
        );
    }

    @Test
    public void disableWaitingForStageJob() {
        // arrange
        StageGroup stageGroup = new StageGroup(STAGE_ID);

        FlowBuilder builder = flowBuilder();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStage(STAGE_ID));
        Flow flow = builder.build();

        FlowLaunchId firstLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowStateService.recalc(
                firstLaunchId,
                new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create())
        );

        FlowLaunchId secondLaunchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        Assertions.assertEquals(
                StatusChangeType.WAITING_FOR_STAGE,
                flowTester.getFlowLaunch(secondLaunchId).getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );

        // act
        flowStateService.disableJobsInLaunchGracefully(secondLaunchId, jobState -> true, false, false);

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(secondLaunchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
    }

    @Test
    public void disableJobsIgnoringUninterruptibleStage() {
        // arrange
        StageGroup stageGroup = new StageGroup(StageBuilder.create(STAGE_ID).uninterruptable());

        FlowBuilder builder = flowBuilder();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStage(STAGE_ID));
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        FlowLaunchEntity launch = flowStateService.recalc(
                launchId,
                new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create())
        );

        flowLaunchSave(launch);

        // act
        flowStateService.disableJobsInLaunchGracefully(launchId, jobState -> true, true, false);
        flowStateService.recalc(launchId, new ExecutorFailedToInterruptEvent(FIRST_JOB_ID, 1));
        flowTester.runScheduledJobsToCompletion();

        // assert
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertTrue(flowLaunch.getJobState(FIRST_JOB_ID).isDisabled());
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                flowLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertNull(flowLaunch.getJobState(SECOND_JOB_ID).getLastLaunch());
    }

    protected static FlowBuilder flowBuilder() {
        FlowBuilder builder = FlowEngineTestBase.flowBuilder();
        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID);
        builder.withJob(DummyJob.ID, SECOND_JOB_ID).withUpstreams(firstJob);
        return builder;
    }

}
