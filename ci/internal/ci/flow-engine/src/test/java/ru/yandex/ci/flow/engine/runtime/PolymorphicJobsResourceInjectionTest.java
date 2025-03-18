package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Producer451Job;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

public class PolymorphicJobsResourceInjectionTest extends FlowEngineTestBase {
    private static final String JOB_ID = "job";

    @Test
    public void simpleInjection() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(getFlow());

        StoredResourceContainer producedResources = flowTester.getProducedResources(flowLaunchId, JOB_ID);

        Res1 res1 = flowTester.getResourceOfType(producedResources, Res1.getDefaultInstance());
        Res2 res2 = flowTester.getResourceOfType(producedResources, Res2.getDefaultInstance());

        Assertions.assertEquals("res1 res1", res1.getS());
        Assertions.assertEquals("res2 res2", res2.getS());
    }

    private Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder producer = builder.withJob(Producer451Job.ID, "j1");

        builder.withJob(ChildJob.ID, JOB_ID)
                .withResources(res1("res1"), res2("res2"))
                .withUpstreams(producer);

        return builder.build();
    }

    @Consume(name = "res1", proto = Res1.class)
    @Produces(single = Res1.class)
    public static class ParentJob implements JobExecutor {
        public static final UUID ID = UUID.fromString("c6f72564-e5e0-4db1-9872-80c8b5b82680");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("res1 " + context.resources().consume(Res1.class).getS()));
        }
    }

    @Consume(name = "res2", proto = Res2.class)
    @Produces(single = Res2.class)
    public static class ChildJob extends ParentJob {
        public static final UUID ID = UUID.fromString("dd4433d4-ce1e-49c9-9671-02c2e9a4be80");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            super.execute(context);
            context.resources().produce(res2("res2 " + context.resources().consume(Res2.class).getS()));
        }
    }
}
