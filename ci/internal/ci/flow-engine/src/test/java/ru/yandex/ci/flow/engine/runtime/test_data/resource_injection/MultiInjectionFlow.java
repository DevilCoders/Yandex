package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection;

import com.google.protobuf.Message;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.MultiResource451SumJob;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Producer451Job;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.ProducerDerived451Job;
import ru.yandex.ci.flow.test.TestFlowId;

public class MultiInjectionFlow {

    public static final FlowFullId FLOW_NAME = TestFlowId.of("multi_injection");
    public static final String SUMMATOR_JOB_ID = "summator";

    private MultiInjectionFlow() {
        //
    }

    public static Flow getFlow(Message... jobResources) {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder producer1 = builder.withJob(Producer451Job.ID, "j1");

        JobBuilder producer2 = builder.withJob(ProducerDerived451Job.ID, "j2")
                .withUpstreams(producer1);

        JobBuilder producer3 = builder.withJob(Producer451Job.ID, "j3")
                .withUpstreams(producer1);

        builder.withJob(MultiResource451SumJob.ID, SUMMATOR_JOB_ID)
                .withResources(jobResources)
                .withUpstreams(producer2, producer3);

        return builder.build();
    }
}
