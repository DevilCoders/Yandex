package ru.yandex.ci.flow.engine.runtime;

import java.time.Instant;

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
import ru.yandex.ci.flow.engine.runtime.events.DisableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.exceptions.FlowDisabledException;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;

public class DisableFlowTest extends FlowEngineTestBase {
    private static final String START_JOB_ID = "START";
    private static final String FIRST_JOB_ID = "FIRST_JOB";
    private static final String SECOND_JOB_ID = "SECOND_JOB";
    private static final String STAGE_GROUP_ID = "STAGE_GROUP_ID";

    @Autowired
    private FlowStateService flowStateService;

    @Test
    public void manualTriggersDoesNotWorkOnDisabledFlow() {
        Assertions.assertThrows(FlowDisabledException.class, () -> {
            // arrange
            FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleFlow.FLOW_ID);
            flowStateService.disableLaunchGracefully(flowLaunchId, false);

            // act
            flowTester.triggerJob(flowLaunchId, SimpleFlow.JOB_ID);
        });
    }

    @Test
    public void manualTriggerDisableDoesNotWorkOnDisabledFlow() {
        Assertions.assertThrows(FlowDisabledException.class, () -> {
            // arrange
            FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleFlow.FLOW_ID);
            flowStateService.disableLaunchGracefully(flowLaunchId, false);

            // act
            flowTester.recalcFlowLaunch(
                    flowLaunchId, new DisableJobManualSwitchEvent(SimpleFlow.JOB_ID, "silly_user", Instant.now())
            );
        });
    }

    @Test
    public void forceSuccessDoesNotWorkOnDisabledFlow() {
        Assertions.assertThrows(FlowDisabledException.class, () -> {
            // arrange
            FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleFlow.FLOW_ID);
            flowStateService.disableLaunchGracefully(flowLaunchId, false);

            // act
            flowTester.recalcFlowLaunch(
                    flowLaunchId, new ForceSuccessTriggerEvent(SimpleFlow.JOB_ID, "silly_user")
            );
        });
    }

    @Test
    public void disableFlowGracefully_ShouldBeginDisabling_IfJobIsQueued() {
        FlowBuilder builder = flowBuilder();
        builder.withJob(DummyJob.ID, FIRST_JOB_ID);
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow);
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertEquals(
                StatusChangeType.QUEUED,
                flowLaunch.getJobState(FIRST_JOB_ID).getLastStatusChangeType()
        );

        flowStateService.disableLaunchGracefully(launchId, false);
        flowLaunch = flowTester.getFlowLaunch(launchId);

        assertDisabling(flowLaunch);
    }

    @Test
    public void disableFlowGracefully_ShouldDisableImmediately_IfNoJobsRunning() {
        FlowBuilder builder = flowBuilder();
        builder.withJob(DummyJob.ID, FIRST_JOB_ID).withManualTrigger();
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow);
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertNull(flowLaunch.getJobState(FIRST_JOB_ID).getLastLaunch());

        flowStateService.disableLaunchGracefully(launchId, false);
        flowLaunch = flowTester.getFlowLaunch(launchId);

        assertDisabled(flowLaunch);
    }

    @Test
    public void disableFlowGracefully_ShouldNotInterruptNonInterruptableStage() {
        StageGroup stageGroup = new StageGroup(StageBuilder.create("first_stage").uninterruptable());

        FlowBuilder builder = twoJobFlow();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStages().get(0));
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowStateService.disableLaunchGracefully(launchId, false);
        flowTester.runScheduledJobToCompletion(FIRST_JOB_ID);

        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        assertDisabling(flowLaunch);

        flowTester.runScheduledJobToCompletion(SECOND_JOB_ID);
        flowLaunch = flowTester.getFlowLaunch(launchId);
        assertDisabled(flowLaunch);
    }

    @Test
    public void disableFlowGracefully_ShouldNotInterruptNonInterruptableStage_IfItStoppedOnManualTrigger() {
        StageGroup stageGroup = new StageGroup(StageBuilder.create("first_stage").uninterruptable());

        FlowBuilder builder = twoJobFlow();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStages().get(0));
        ((JobBuilder) builder.getJobBuilder(SECOND_JOB_ID)).withManualTrigger();
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowStateService.disableLaunchGracefully(launchId, false);
        flowTester.runScheduledJobsToCompletion();

        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        assertDisabling(flowLaunch);

        flowTester.triggerJob(launchId, SECOND_JOB_ID);
        flowTester.runScheduledJobsToCompletion();
        flowLaunch = flowTester.getFlowLaunch(launchId);
        assertDisabled(flowLaunch);
    }

    @Test
    public void disableFlowGracefully_ShouldInterruptNonInterruptableStage_IfManualInterruption() {
        StageGroup stageGroup = new StageGroup(StageBuilder.create("first_stage").uninterruptable());

        FlowBuilder builder = twoJobFlow();
        builder.getJobBuilder(FIRST_JOB_ID).beginStage(stageGroup.getStages().get(0));
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow, STAGE_GROUP_ID);
        flowStateService.disableLaunchGracefully(launchId, true);
        flowTester.runScheduledJobToCompletion(FIRST_JOB_ID);

        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        assertDisabled(flowLaunch);
    }

    @Test
    public void cancelGracefulDisabling() {
        FlowBuilder builder = twoJobFlow();
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow);
        flowStateService.disableLaunchGracefully(launchId, false);
        flowStateService.cancelGracefulDisabling(launchId);

        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertFalse(flowLaunch.isDisablingGracefully());
        Assertions.assertFalse(flowLaunch.isDisabled());
    }

    @Test
    public void cancelGracefulDisabling_ShouldThrowException_IfAlreadyDisabled() {
        Assertions.assertThrows(IllegalStateException.class, () -> {

            FlowBuilder builder = twoJobFlow();
            Flow flow = builder.build();

            FlowLaunchId launchId = flowTester.activateLaunch(flow);
            flowTester.runScheduledJobsToCompletion();
            flowStateService.disableLaunchGracefully(launchId, false);

            FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
            Assertions.assertTrue(flowLaunch.isDisabled());

            flowStateService.cancelGracefulDisabling(launchId);
        });
    }

    @Test
    public void disableJobsInLaunchGracefullyForParallelJobs() {
        FlowBuilder builder = parallelFlow();
        Flow flow = builder.build();

        FlowLaunchId launchId = flowTester.activateLaunch(flow);
        flowTester.runScheduledJobToCompletion(START_JOB_ID);
        flowTester.runScheduledJobToCompletion(FIRST_JOB_ID);
        flowTester.runScheduledJobToCompletion(SECOND_JOB_ID);

        flowStateService.disableJobsInLaunchGracefully(launchId, job -> true, true, false);
        flowTester.runScheduledJobToCompletion(FIRST_JOB_ID);
        flowTester.runScheduledJobToCompletion(SECOND_JOB_ID);

        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(launchId);
        Assertions.assertEquals(
                StatusChangeType.KILLED,
                flowLaunch.getJobs().get(FIRST_JOB_ID).getLastStatusChangeType()
        );
        Assertions.assertEquals(
                StatusChangeType.KILLED,
                flowLaunch.getJobs().get(SECOND_JOB_ID).getLastStatusChangeType()
        );
    }

    private FlowBuilder twoJobFlow() {
        FlowBuilder builder = flowBuilder();
        JobBuilder firstJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID);
        builder.withJob(DummyJob.ID, SECOND_JOB_ID).withUpstreams(firstJob);
        return builder;
    }


    private FlowBuilder parallelFlow() {
        FlowBuilder builder = flowBuilder();
        JobBuilder singleJob = builder.withJob(DummyJob.ID, START_JOB_ID);

        builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .withUpstreams(singleJob)
                .withScheduler()
                .build();
        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(singleJob)
                .withScheduler()
                .build();

        return builder;
    }

    private void assertDisabled(FlowLaunchEntity flowLaunch) {
        Assertions.assertFalse(flowLaunch.isDisablingGracefully());
        Assertions.assertTrue(flowLaunch.isDisabled());
    }

    private void assertDisabling(FlowLaunchEntity flowLaunch) {
        Assertions.assertTrue(flowLaunch.isDisablingGracefully());
        Assertions.assertFalse(flowLaunch.isDisabled());
    }

}
