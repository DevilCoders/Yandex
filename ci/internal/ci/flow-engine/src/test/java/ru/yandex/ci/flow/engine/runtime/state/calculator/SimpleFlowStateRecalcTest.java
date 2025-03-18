package ru.yandex.ci.flow.engine.runtime.state.calculator;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;

public class SimpleFlowStateRecalcTest extends FlowStateCalculatorTest {
    private static final String JOB_ID = "dummy";

    @Override
    protected Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(DummyJob.ID, JOB_ID)
                .withManualTrigger();

        return builder.build();
    }


    @Test
    public void triggerJob() {
        recalc(null);
        recalc(new TriggerEvent(SimpleFlow.JOB_ID, USERNAME, false));

        String triggeredJobId = getTriggeredJobs().get(0).getJobLaunchId().getJobId();
        Assertions.assertEquals(SimpleFlow.JOB_ID, triggeredJobId);
        JobState jobState = getFlowLaunch().getJobs().get(SimpleFlow.JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.QUEUED,
                jobState.getLastStatusChangeType()
        );
    }

    @Test
    public void jobRunningEvent() {
        recalc(null);
        recalc(new TriggerEvent(SimpleFlow.JOB_ID, USERNAME, false));

        TestJobScheduler jobScheduler = new TestJobScheduler(db);
        recalc(new JobRunningEvent(SimpleFlow.JOB_ID, 1, DummyTmsTaskIdFactory.create()));

        Assertions.assertTrue(jobScheduler.getTriggeredJobs().isEmpty());

        JobState jobState = getFlowLaunch().getJobs().get(SimpleFlow.JOB_ID);
        Assertions.assertEquals(
                StatusChangeType.RUNNING,
                jobState.getLastStatusChangeType()
        );
    }

    @Test
    public void jobSucceededEvent() {
        recalc(null);
        recalc(new TriggerEvent(SimpleFlow.JOB_ID, USERNAME, false));
        recalc(new JobRunningEvent(SimpleFlow.JOB_ID, 1, DummyTmsTaskIdFactory.create()));

        TestJobScheduler jobScheduler = new TestJobScheduler(db);
        recalc(new JobExecutorSucceededEvent(SimpleFlow.JOB_ID, 1));
        recalc(new SubscribersSucceededEvent(SimpleFlow.JOB_ID, 1));
        recalc(new JobSucceededEvent(SimpleFlow.JOB_ID, 1));

        Assertions.assertTrue(jobScheduler.getTriggeredJobs().isEmpty());

        JobState jobState = getFlowLaunch().getJobs().get(SimpleFlow.JOB_ID);
        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                jobState.getLastStatusChangeType()
        );
    }

    @Test
    public void jobForceSucceededEvent() {
        recalc(null);
        recalc(new TriggerEvent(SimpleFlow.JOB_ID, USERNAME, false));
        recalc(new JobRunningEvent(SimpleFlow.JOB_ID, 1, DummyTmsTaskIdFactory.create()));

        TestJobScheduler jobScheduler = new TestJobScheduler(db);
        recalc(new JobExecutorFailedEvent(SimpleFlow.JOB_ID, 1, ResourceRefContainer.empty(), new RuntimeException()));
        recalc(new SubscribersSucceededEvent(SimpleFlow.JOB_ID, 1));
        recalc(new JobFailedEvent(SimpleFlow.JOB_ID, 1));
        Assertions.assertEquals(
                StatusChangeType.FAILED,
                getFlowLaunch().getJobs().get(SimpleFlow.JOB_ID).getLastStatusChangeType()
        );

        recalc(new ForceSuccessTriggerEvent(SimpleFlow.JOB_ID, "jenkl"));

        Assertions.assertTrue(jobScheduler.getTriggeredJobs().isEmpty());

        JobState jobState = getFlowLaunch().getJobs().get(SimpleFlow.JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.FORCED_EXECUTOR_SUCCEEDED,
                jobState.getLastStatusChangeType()
        );

        Assertions.assertEquals(jobState.getLastLaunch().getForceSuccessTriggeredBy(), "jenkl");
    }

}
