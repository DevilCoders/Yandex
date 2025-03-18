package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.test.TestFlowId;

public class JobForceSucceededTest extends FlowEngineTestBase {
    private static final String STAGE_GROUP_ID = "jobForceSucceededTest";

    private static final String FIRST_JOB_ID = "first";
    private static final String SECOND_JOB_ID = "second";
    private static final String THIRD_JOB_ID = "third";
    private static final String AFTER_FAILED_JOB_ID = "after_failed";
    private static final String FOURTH_JOB_ID = "fourth";
    private static final String FINAL_JOB_ID = "final";

    private static final String FIRST_STAGE = "first";
    private static final String SECOND_STAGE = "second";

    @Autowired
    private FlowStateService flowStateService;

    private final StageGroup stageGroup = new StageGroup(FIRST_STAGE, SECOND_STAGE);

    private FlowFullId flowId;

    @Test
    public void forceSuccessJobWithoutProducedResources() {
        prepareFlowLaunch(flowWithFailedJob());

        FlowLaunchId flowLunchId = flowTester.runFlowToCompletion(flowId);
        JobLaunch firstJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, FIRST_JOB_ID);
        Assertions.assertEquals(firstJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.FAILED);

        flowStateService.recalc(flowLunchId, new ForceSuccessTriggerEvent(FIRST_JOB_ID, "jenkl"));
        flowTester.runScheduledJobsToCompletion();

        firstJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, FIRST_JOB_ID);
        Assertions.assertEquals(firstJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.SUCCESSFUL);

        JobLaunch secondJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, SECOND_JOB_ID);
        Assertions.assertEquals(secondJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.SUCCESSFUL);
    }

    @Test
    void forceSuccessJobLeftBehind() {
        var flow = flowWithFailedJobAndAny();
        FlowLaunchId flowLunchId = flowTester.runFlowToCompletion(flow, STAGE_GROUP_ID);

        JobLaunch thirdJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, THIRD_JOB_ID);
        Assertions.assertEquals(thirdJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.FAILED);

        JobLaunch fourthJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, FOURTH_JOB_ID);
        Assertions.assertEquals(fourthJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.SUCCESSFUL);

        Assertions.assertNull(flowTester.getJobLastLaunch(flowLunchId, AFTER_FAILED_JOB_ID));

        flowStateService.recalc(flowLunchId, new ForceSuccessTriggerEvent(THIRD_JOB_ID, "jenkl"));
        flowTester.runScheduledJobsToCompletion();

        thirdJobLastLaunch = flowTester.getJobLastLaunch(flowLunchId, THIRD_JOB_ID);
        Assertions.assertEquals(thirdJobLastLaunch.getLastStatusChange().getType(), StatusChangeType.SUCCESSFUL);

        var afterFailed = flowTester.getFlowLaunch(flowLunchId).getJobState(AFTER_FAILED_JOB_ID);
        Assertions.assertFalse(afterFailed.isReadyToRun());
        Assertions.assertTrue(afterFailed.getLaunches().isEmpty());
    }

    private void prepareFlowLaunch(Flow flow) {
        flowId = flowTester.register(flow, TestFlowId.TEST_PATH);
    }

    private Flow flowWithFailedJob() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder first = builder.withJob(FailedJob.ID, FIRST_JOB_ID);

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(first);

        return builder.build();
    }

    /**
     * <pre>
     *           ┌────► SECOND ───────────────────────────┐
     *           │                                        │any          manual
     * FIRST  ───┤                                        ├────► FOURTH ───────► FINAL
     *           │                                        │
     *           └────► THIRD (FAILED) ──► AFTER_FAILED ──┘
     * </pre>
     */
    private Flow flowWithFailedJobAndAny() {
        FlowBuilder builder = flowBuilder();

        var first = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .beginStage(stageGroup.getStage(FIRST_STAGE));

        var second = builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(first);

        var third = builder.withJob(FailedJob.ID, THIRD_JOB_ID)
                .withUpstreams(first);

        var afterFailed = builder.withJob(DummyJob.ID, AFTER_FAILED_JOB_ID)
                .withUpstreams(third);

        var fourth = builder.withJob(DummyJob.ID, FOURTH_JOB_ID)
                .beginStage(stageGroup.getStage(SECOND_STAGE))
                .withUpstreams(CanRunWhen.ANY_COMPLETED, second, afterFailed);

        builder.withJob(DummyJob.ID, FINAL_JOB_ID)
                .withManualTrigger()
                .withUpstreams(fourth);

        return builder.build();
    }

    public static class FailedJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("c6489565-bad8-4743-9fd7-40410fe9fd0d");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            throw new RuntimeException();
        }
    }

}
