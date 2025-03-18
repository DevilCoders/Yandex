package ru.yandex.ci.flow.engine.runtime.test_data.independent;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.test.TestFlowId;

public class IndependentFlow {
    public static final FlowFullId FLOW_ID = TestFlowId.of("independent");
    public static final String JOB_PREFIX = "job";
    public static final int JOB_COUNT = 50;

    private IndependentFlow() {
        //
    }

    public static Flow flow() {
        FlowBuilder builder = FlowBuilder.create();

        for (int i = 0; i < JOB_COUNT; ++i) {
            builder.withJob(DummyJob.ID, JOB_PREFIX + i);
        }

        return builder.build();
    }
}
