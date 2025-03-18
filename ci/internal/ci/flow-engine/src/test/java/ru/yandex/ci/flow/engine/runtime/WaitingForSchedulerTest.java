package ru.yandex.ci.flow.engine.runtime;

import java.time.Instant;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.ScheduleChangeEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.ToggleSchedulerConstraintModifyEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculatorTestBase;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.test.TestFlowId;

public class WaitingForSchedulerTest extends FlowStateCalculatorTestBase {
    private static final String FIRST_JOB = "first";
    private static final String SECOND_JOB = "second";
    private static final String FIRST_STAGE = "first";
    private static final String SECOND_STAGE = "second";
    private static final String STAGE_GROUP_ID = "test-stages";

    private final StageGroup stageGroup = new StageGroup(FIRST_STAGE, SECOND_STAGE);

    @Test
    public void blocksExecution() {
        var flowId = flowProvider.register(flow(false, false), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(1, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(1, testJobWaitingScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(
                StatusChangeType.WAITING_FOR_SCHEDULE,
                flowTester.getFlowLaunch(launchId).getJobState(SECOND_JOB).getLastStatusChangeType()
        );

        flowStateService.recalc(launchId, new ScheduleChangeEvent(SECOND_JOB));
        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
    }

    @Test
    public void triggerEventTest() {
        var flowId = flowProvider.register(flow(false, false), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(1, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(1, testJobWaitingScheduler.getTriggeredJobs().size());

        flowStateService.recalc(launchId, new TriggerEvent(SECOND_JOB, "test_user", false));
        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(1, flowLaunchGet(launchId).getJobState(SECOND_JOB).getLaunches().size());
        Assertions.assertEquals(
                "test_user",
                flowLaunchGet(launchId).getJobState(SECOND_JOB).getLastLaunch().getTriggeredBy()
        );
    }

    @Test
    public void toggleSchedulerConstraintTest() {
        var flowId = flowProvider.register(flow(false, false), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(
                launchId,
                new ToggleSchedulerConstraintModifyEvent(SECOND_JOB, USERNAME, Instant.now())
        );
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(0, testJobWaitingScheduler.getTriggeredJobs().size());
    }

    @Test
    public void schedulerForStageTest() throws AYamlValidationException {
        var flowId = flowProvider.register(flow(true, false), TestFlowId.TEST_PATH);
        Flow flow = flowProvider.get(flowId);
        flowTester.createStageGroupState(STAGE_GROUP_ID, flow.getStages());
        FlowLaunchId launchId = activateLaunch(flowId, STAGE_GROUP_ID);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(1, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(1, testJobWaitingScheduler.getTriggeredJobs().size());

        flowStateService.recalc(launchId, new ScheduleChangeEvent(SECOND_JOB));
        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
    }

    @Test
    public void skipSchedulerForManualTriggerTest() throws AYamlValidationException {
        var flowId = flowProvider.register(flow(true, true), TestFlowId.TEST_PATH);
        Flow flow = flowProvider.get(flowId);
        flowTester.createStageGroupState(STAGE_GROUP_ID, flow.getStages());
        FlowLaunchId launchId = activateLaunch(flowId, STAGE_GROUP_ID);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(1, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(0, testJobWaitingScheduler.getTriggeredJobs().size());

        flowStateService.recalc(launchId, new TriggerEvent(SECOND_JOB, "test_user", false));

        flowStateService.recalc(launchId, new ScheduleChangeEvent(SECOND_JOB));
        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(0, testJobWaitingScheduler.getTriggeredJobs().size());
    }

    @Test
    public void schedulerForManualTriggerTest() {
        var flowId = flowProvider.register(flow(false, true), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        Assertions.assertEquals(1, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(0, testJobWaitingScheduler.getTriggeredJobs().size());

        flowStateService.recalc(launchId, new TriggerEvent(SECOND_JOB, "test_user", false));

        flowStateService.recalc(launchId, new ScheduleChangeEvent(SECOND_JOB));
        Assertions.assertEquals(2, testJobScheduler.getTriggeredJobs().size());
        Assertions.assertEquals(0, testJobWaitingScheduler.getTriggeredJobs().size());
    }

    private Flow flow(boolean staged, boolean manualTrigger) {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder first = builder.withJob(DummyJob.ID, FIRST_JOB);
        if (staged) {
            first.beginStage(stageGroup.getStage(FIRST_STAGE));
        }

        JobBuilder second = builder.withJob(DummyJob.ID, SECOND_JOB)
                .withUpstreams(first)
                .withScheduler()
                .build();

        if (staged) {
            second.beginStage(stageGroup.getStage(SECOND_STAGE));
        }

        if (manualTrigger) {
            second.withManualTrigger();
        }

        return builder.build();
    }

}
