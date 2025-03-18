package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Resource451DoubleJob;
import ru.yandex.ci.flow.test.TestFlowId;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.resource451;

public class StaticResourcesFlow {
    public static final FlowFullId FLOW_NAME = TestFlowId.of("static_resources");
    public static final String DOUBLER_JOB_ID = "doubler";

    private StaticResourcesFlow() {
        //
    }

    public static Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(Resource451DoubleJob.ID, DOUBLER_JOB_ID)
                .withResources(resource451(451));

        return builder.build();
    }
}
