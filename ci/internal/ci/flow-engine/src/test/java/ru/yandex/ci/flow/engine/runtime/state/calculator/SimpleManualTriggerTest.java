package ru.yandex.ci.flow.engine.runtime.state.calculator;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

import static org.assertj.core.api.Assertions.assertThat;

public class SimpleManualTriggerTest extends FlowStateCalculatorTest {
    @Override
    protected Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(DummyJob.ID, "dummy")
                .withManualTrigger();

        return builder.build();
    }

    @Test
    public void manualJobNotTriggeredAutomatically() {
        recalc(null);

        JobState singleJobState = getFlowLaunch().getJobs().values().iterator().next();
        Assertions.assertTrue(singleJobState.isReadyToRun());
        assertThat(getTriggeredJobs()).isEmpty();
    }

}
