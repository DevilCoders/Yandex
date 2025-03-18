package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.ExpectedResources;
import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.expectedResources;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

public class OptionalResourcesTest extends FlowEngineTestBase {
    public static final String JOB_ID = "job";

    @Test
    public void testNoResourcesProvidedException() {
        //TODO check for exception when validation is ready
        testFlow(null, null, expectedResources(null, null), StatusChangeType.FAILED);
    }

    @Test
    public void testDefaultResource() {
        testFlow("res1", null, expectedResources("res1", "default"), StatusChangeType.SUCCESSFUL);
    }

    @Test
    public void testDefaultResourceOverride() {
        testFlow("res11", "res2", expectedResources("res11", "res2"), StatusChangeType.SUCCESSFUL);
    }

    private void testFlow(
            @Nullable String res1,
            @Nullable String res2,
            ExpectedResources expectedResources,
            StatusChangeType expectedState
    ) {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(flow(res1, res2, expectedResources));
        JobLaunch launch = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        Assertions.assertEquals(
                expectedState,
                launch.getLastStatusChange().getType()
        );
    }

    private Flow flow(@Nullable String res1, @Nullable String res2, ExpectedResources expectedResources) {
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder job = builder.withJob(OptionalResourceJob.ID, JOB_ID);
        job.withResources(expectedResources);
        if (res1 != null) {
            job.withResources(res1(res1));
        }
        if (res2 != null) {
            job.withResources(res2(res2));
        }
        return builder.build();
    }

    @Consume(name = "res1", proto = Res1.class)
    @Consume(name = "res2", proto = Res2.class, list = true)
    @Consume(name = "expected_resources", proto = ExpectedResources.class)
    public static class OptionalResourceJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("dcfe6a8c-05c1-41d1-9db5-f581720d9ccf");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            var expectedResources = context.resources().consume(ExpectedResources.class);
            var res1 = context.resources().consume(Res1.class);
            var res2 = context.resources().consumeList(Res2.class);
            Assertions.assertEquals(expectedResources.getRes1(), res1);
            Assertions.assertEquals(expectedResources.getRes2(), res2.isEmpty() ? res2("default") : res2.get(0));
        }
    }
}
