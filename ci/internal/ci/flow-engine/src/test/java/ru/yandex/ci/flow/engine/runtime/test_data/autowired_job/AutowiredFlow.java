package ru.yandex.ci.flow.engine.runtime.test_data.autowired_job;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.test.TestFlowId;

public final class AutowiredFlow {
    public static final String JOB_ID = "job";
    public static final FlowFullId FLOW_ID = TestFlowId.of("autowired");

    private AutowiredFlow() {
        //
    }

    public static Flow flow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(AutowiredJob.ID, JOB_ID);

        return builder.build();
    }
}
