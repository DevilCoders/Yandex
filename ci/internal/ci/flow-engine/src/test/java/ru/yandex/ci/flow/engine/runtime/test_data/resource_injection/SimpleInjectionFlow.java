package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection;

import com.google.protobuf.Message;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Producer451Job;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Resource451DoubleJob;
import ru.yandex.ci.flow.test.TestFlowId;

public class SimpleInjectionFlow {
    public static final FlowFullId FLOW_NAME = TestFlowId.of("simple_injection");
    public static final String DOUBLER_JOB_ID = "downstream";

    private SimpleInjectionFlow() {
        //
    }

    public static Flow getFlow(Message... jobResources) {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder producer = builder.withJob(Producer451Job.ID, "j1");

        builder.withJob(Resource451DoubleJob.ID, DOUBLER_JOB_ID)
                .withResources(jobResources)
                .withUpstreams(producer);

        return builder.build();
    }

}
