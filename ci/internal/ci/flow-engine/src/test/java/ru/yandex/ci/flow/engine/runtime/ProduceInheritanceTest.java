package ru.yandex.ci.flow.engine.runtime;

import java.util.List;
import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRef;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;


public class ProduceInheritanceTest extends FlowEngineTestBase {
    public static final String JOB_ID = "job";

    @Autowired
    private SourceCodeService sourceCodeService;

    @Test
    public void testProducesWhenNoAnnotationOnNested() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow());

        JobLaunch jobLastLaunch = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        List<ResourceRef> resources = jobLastLaunch.getProducedResources().getResources();
        Assertions.assertEquals(resources.size(), 1);
    }

    @Test
    public void testMergesProduces() {
        JobExecutorObject executor = sourceCodeService.getJobExecutor(
                ExecutorContext.internal(InternalExecutorContext.of(InheritedJobWithTwoResourceProduces.ID))
        );
        Assertions.assertNotNull(executor);
        Assertions.assertEquals(2, executor.getProducedResources().size());
    }

    private Flow flow() {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(InheritedJob.ID, JOB_ID);
        return builder.build();
    }

    @Produces(single = Res1.class)
    public abstract static class BaseJob implements JobExecutor {
        @Override
        public UUID getSourceCodeId() {
            return UUID.fromString("2ccc01c6-baa4-465b-a5bd-5d60db2ecd9a");
        }
    }

    public static class InheritedJob extends BaseJob {

        public static final UUID ID = UUID.fromString("2ccc01c6-baa4-465b-a5bd-5d60db2ecd9a");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("1"));
        }
    }

    @Produces(single = {Res1.class, Res2.class})
    public static class InheritedJobWithTwoResourceProduces extends BaseJob {
        private static final UUID ID = UUID.fromString("e50177ae-5620-4cff-b3c3-b3ddd36f2843");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("1"));
            context.resources().produce(res2("2"));
        }
    }
}
