package ru.yandex.ci.flow.engine.runtime.state.calculator;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

public class TwoNodeFlowStateRecalcTest extends FlowStateCalculatorTest {
    private static final String FIRST_JOB_ID = "first";
    private static final String SECOND_JOB_ID = "second";

    @Override
    protected Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder first = builder.withJob(DummyJob.ID, FIRST_JOB_ID)
                .withManualTrigger();

        builder.withJob(DummyJob.ID, SECOND_JOB_ID)
                .withUpstreams(first);

        return builder.build();
    }

    @Test
    public void triggerFirstJob() {
        recalc(null);
        recalc(new TriggerEvent(FIRST_JOB_ID, USERNAME, false));

        Assertions.assertEquals(1, getTriggeredJobs().size());
        Assertions.assertEquals(FIRST_JOB_ID, getTriggeredJobs().get(0).getJobLaunchId().getJobId());

        JobState firstJobState = getFlowLaunch().getJobs().get(FIRST_JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.QUEUED,
                firstJobState.getLastStatusChangeType()
        );
    }

    @Test
    public void secondJobAutoTrigger() {
        recalc(null);
        recalc(new TriggerEvent(FIRST_JOB_ID, USERNAME, false));

        getTriggeredJobs().clear();
        recalc(new JobRunningEvent(FIRST_JOB_ID, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(FIRST_JOB_ID, 1));
        recalc(new SubscribersSucceededEvent(FIRST_JOB_ID, 1));
        recalc(new JobSucceededEvent(FIRST_JOB_ID, 1));

        Assertions.assertTrue(getFlowLaunch().getJobState(FIRST_JOB_ID).isReadyToRun());
        Assertions.assertEquals(1, getTriggeredJobs().size());
        Assertions.assertEquals(SECOND_JOB_ID, getTriggeredJobs().get(0).getJobLaunchId().getJobId());

        JobState secondJobState = getFlowLaunch().getJobState(SECOND_JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.QUEUED,
                secondJobState.getLastStatusChangeType()
        );
    }
}
