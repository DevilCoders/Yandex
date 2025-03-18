package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;

public class ProduceMultipleResourcesTest extends FlowEngineTestBase {
    public static final String JOB_ID = "job";

    @Test
    public void produceZeroResourcesIsOK() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow(ZeroResourcesJob.ID));

        JobLaunch jobLastLaunch = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                jobLastLaunch.getLastStatusChange().getType()
        );
    }

    @Test
    public void producesTwoResources() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow(TwoResourcesJob.ID));

        StoredResourceContainer producedResources = flowTester.getProducedResources(flowLaunchId, JOB_ID);
        Assertions.assertEquals(2, producedResources.getResources().size());
    }

    private Flow flow(UUID executorId) {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(executorId, JOB_ID);
        return builder.build();
    }

    @Produces(multiple = Res1.class)
    public static class ZeroResourcesJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("3d77c82c-e667-484d-961e-d6cbc905041a");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            // this job could provide Res1, but it doesn't want to
        }
    }

    @Produces(multiple = Res1.class)
    public static class TwoResourcesJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("d9afe8fe-a0fa-4390-9a04-1e984d9d9fd4");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("x1"));
            context.resources().produce(res1("x2"));
        }
    }
}
