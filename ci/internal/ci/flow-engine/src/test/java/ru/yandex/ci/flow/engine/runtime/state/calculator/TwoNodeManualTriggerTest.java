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
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;

import static org.assertj.core.api.Assertions.assertThat;

public class TwoNodeManualTriggerTest extends FlowStateCalculatorTest {
    private static final String START_JOB = "start";
    private static final String END_JOB = "end";

    @Override
    protected Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder start = builder.withJob(DummyJob.ID, START_JOB);

        builder.withJob(DummyJob.ID, END_JOB)
                .withUpstreams(start)
                .withManualTrigger();

        return builder.build();
    }

    @Test
    public void manualJobNotTriggeredAutomatically() {
        recalc(null);
        getTriggeredJobs().clear();

        Assertions.assertFalse(getFlowLaunch().getJobState(END_JOB).isReadyToRun());

        recalc(new JobRunningEvent(START_JOB, 1, DummyTmsTaskIdFactory.create()));
        recalc(new JobExecutorSucceededEvent(START_JOB, 1));
        recalc(new SubscribersSucceededEvent(START_JOB, 1));
        recalc(new JobSucceededEvent(START_JOB, 1));

        assertThat(getTriggeredJobs()).isEmpty();
        assertThat(getFlowLaunch().getJobState(END_JOB).isReadyToRun()).isTrue();
    }

}
