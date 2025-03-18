package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

public class ClassNotFoundTest extends FlowEngineTestBase {
    public static final String FIRST_JOB_ID = "dummy1";
    public static final String CLASS_NOT_FOUND_JOB_ID = "dummyClassNotFound";

    @Test
    public void classNotFoundTest() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(classNotFoundFlow());
        FlowLaunchEntity flowLaunch = flowTester.getFlowLaunch(flowLaunchId);
        flowLaunch.getJobState(CLASS_NOT_FOUND_JOB_ID).setExecutorContextInternal(
                InternalExecutorContext.of(UUID.fromString("1f6f8c6e-a212-4898-ae2e-d12a43127d7b")));

        flowTester.saveFlowLaunch(flowLaunch);
        flowTester.triggerJob(flowLaunchId, FIRST_JOB_ID);
        flowTester.runScheduledJobsToCompletion();

        JobLaunch jobLaunch = flowTester.getJobLastLaunch(flowLaunchId, CLASS_NOT_FOUND_JOB_ID);

        Assertions.assertEquals(
                StatusChangeType.FAILED,
                jobLaunch.getLastStatusChange().getType()
        );

        Assertions.assertTrue(jobLaunch.getExecutionExceptionStacktrace().startsWith("Class not found"));
        Assertions.assertTrue(
                flowTester.getFlowLaunch(flowLaunchId).getJobState(CLASS_NOT_FOUND_JOB_ID).isReadyToRun()
        );
    }

    public Flow classNotFoundFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder dummyJob = builder.withJob(DummyJob.ID, FIRST_JOB_ID).withManualTrigger();

        builder.withJob(DummyJob.ID, CLASS_NOT_FOUND_JOB_ID).withUpstreams(dummyJob)
                .withDescription("Для проверки необходимо в ресурсах поменять класс на несуществующий");

        return builder.build();
    }
}
