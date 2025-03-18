package ru.yandex.ci.flow.engine.runtime.test_data.simple;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.test.TestFlowId;

public class SimpleFlow {

    public static final String JOB_ID = "dummy";
    public static final FlowFullId FLOW_ID = TestFlowId.of("simple");

    private SimpleFlow() {
        //
    }

    public static Flow flow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(DummyJob.ID, JOB_ID);

        return builder.build();
    }
}
