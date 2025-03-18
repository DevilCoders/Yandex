package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

public class JobContextProducedResourcesValidationTest extends FlowEngineTestBase {

    @Test
    public void jobThatDeclaresAndProducesSingleRes1_shouldSucceed() {
        assertSuccessfullyProducesResources(JobThatDeclaresAndProducesSingleRes1.ID, 1);
    }

    @Test
    public void jobThatDeclaresSingleRes1ButDoesNotProduceAnything_shouldFail() {
        assertFails(JobThatDeclaresSingleRes1ButDoesNotProduceAnything.ID);
    }

    @Test
    public void jobThatDeclaresSingleRes1ButProducesMultipleRes1_shouldFail() {
        assertFails(JobThatDeclaresSingleRes1ButProducesMultipleRes1.ID);
    }

    @Test
    public void jobThatDeclaresAndProducesMultipleRes1_shouldSucceed() {
        assertSuccessfullyProducesResources(JobThatDeclaresAndProducesMultipleRes1.ID, 2);
    }

    @Test
    public void jobThatProducesUndeclaredResource_shouldFail() {
        assertFails(JobThatProducesUndeclaredResource.ID);
    }

    private void assertSuccessfullyProducesResources(UUID jobId, int expectedResourceCount) {
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder job = builder.withJob(jobId, "j" + jobId);

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(builder.build());

        assertEquals(
                expectedResourceCount,
                flowTester.getProducedResources(flowLaunchId, job.getId()).getResources().size()
        );
    }

    private void assertFails(UUID jobId) {
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder job = builder.withJob(jobId, "j" + jobId);

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(builder.build());

        assertEquals(
                StatusChangeType.FAILED,
                flowTester.getJobLastLaunch(flowLaunchId, job.getId()).getLastStatusChange().getType()
        );
    }


    @Produces(single = Res1.class)
    public static class JobThatDeclaresAndProducesSingleRes1 implements JobExecutor {

        public static final UUID ID = UUID.fromString("72602df4-ac1f-47be-a22f-c8e9641e8593");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1(""));
        }
    }

    @Produces(single = Res1.class)
    public static class JobThatDeclaresSingleRes1ButDoesNotProduceAnything implements JobExecutor {

        public static final UUID ID = UUID.fromString("b7ddafa5-3f28-47c2-ae92-d083798ac331");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("1"));
            context.resources().produce(res1("2"));
        }
    }

    @Produces(single = Res1.class)
    public static class JobThatDeclaresSingleRes1ButProducesMultipleRes1 implements JobExecutor {

        public static final UUID ID = UUID.fromString("13341407-f733-4505-b7de-020f59cb92f0");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("1"));
            context.resources().produce(res1("2"));
        }
    }

    @Produces(multiple = Res1.class)
    public static class JobThatDeclaresAndProducesMultipleRes1 implements JobExecutor {

        public static final UUID ID = UUID.fromString("deacf3ca-ea2f-489c-9a04-5b1af356873b");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            context.resources().produce(res1("1"));
            context.resources().produce(res1("2"));
        }
    }

    @Produces(single = Res1.class)
    public static class JobThatProducesUndeclaredResource implements JobExecutor {

        public static final UUID ID = UUID.fromString("d617a81c-5702-48fc-b88c-8aee7e51a051");

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
