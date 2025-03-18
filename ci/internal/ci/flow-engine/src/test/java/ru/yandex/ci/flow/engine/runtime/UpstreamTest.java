package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.core.test.resources.Res3;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

public class UpstreamTest extends FlowEngineTestBase {

    @Test
    public void noResourceTest() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(TestUpstreamsFlows.noResourceFlow());
        JobLaunch consumeRes123 = flowTester.getJobLastLaunch(flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID);
        StoredResourceContainer storedResourceContainer = flowTester.getConsumedResources(
                flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID
        );

        Assertions.assertEquals(StatusChangeType.SUCCESSFUL, consumeRes123.getLastStatusChange().getType());
        Assertions.assertEquals(0, storedResourceContainer.getResources().size());
    }

    @Test
    public void directResourceSequenceTest() {
        FlowLaunchId flowLaunchId =
                flowTester.runFlowToCompletion(TestUpstreamsFlows.directResourceSequenceFlow());
        JobLaunch consumeRes123 = flowTester.getJobLastLaunch(flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID);
        StoredResourceContainer storedResourceContainer = flowTester.getConsumedResources(
                flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID
        );

        Assertions.assertEquals(StatusChangeType.SUCCESSFUL, consumeRes123.getLastStatusChange().getType());
        Assertions.assertEquals(1, storedResourceContainer.getResources().size());
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res2.class)));
    }

    @Test
    public void directResourceSequenceWithDownstreamTest() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(
                TestUpstreamsFlows.directResourceSequenceWithDownstreamFlow()
        );
        JobLaunch consumeRes123 = flowTester.getJobLastLaunch(flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID);
        StoredResourceContainer storedResourceContainer = flowTester.getConsumedResources(
                flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID
        );

        Assertions.assertEquals(StatusChangeType.SUCCESSFUL, consumeRes123.getLastStatusChange().getType());
        Assertions.assertEquals(2, storedResourceContainer.getResources().size());
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res1.class)));
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res2.class)));
    }

    @Test
    public void directResourceParallelTest() {
        FlowLaunchId flowLaunchId =
                flowTester.runFlowToCompletion(TestUpstreamsFlows.directResourceParallelFlow());
        JobLaunch consumeRes123 = flowTester.getJobLastLaunch(flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID);
        StoredResourceContainer storedResourceContainer = flowTester.getConsumedResources(
                flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID
        );

        Assertions.assertEquals(StatusChangeType.SUCCESSFUL, consumeRes123.getLastStatusChange().getType());
        Assertions.assertEquals(1, storedResourceContainer.getResources().size());
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res3.class)));
    }

    @Test
    public void allResourceTest() {
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(TestUpstreamsFlows.allResourceFlow());
        JobLaunch consumeRes123 = flowTester.getJobLastLaunch(flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID);
        StoredResourceContainer storedResourceContainer = flowTester.getConsumedResources(
                flowLaunchId, TestUpstreamsFlows.CONSUME_RES1_ID
        );

        Assertions.assertEquals(StatusChangeType.SUCCESSFUL, consumeRes123.getLastStatusChange().getType());
        Assertions.assertEquals(2, storedResourceContainer.getResources().size());
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res1.class)));
        Assertions.assertTrue(storedResourceContainer.containsResource(JobResourceType.ofMessageClass(Res2.class)));
    }
}
