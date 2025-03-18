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
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

public class DeclaredResourcesNotProducedTest extends FlowEngineTestBase {
    public static final String JOB_ID = "job";

    @Test
    public void test() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(invalidFlow());

        JobLaunch jobLastLaunch = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);

        Assertions.assertEquals(StatusChangeType.FAILED, jobLastLaunch.getLastStatusChange().getType());
    }

    private Flow invalidFlow() {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(InvalidJob.ID, JOB_ID);
        return builder.build();
    }

    @Produces(single = Res1.class)
    public static class InvalidJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("30572e1f-0a40-423d-b9a3-c41f163e1318");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            // this job should provide Res1, but it doesn't want to
        }
    }
}
