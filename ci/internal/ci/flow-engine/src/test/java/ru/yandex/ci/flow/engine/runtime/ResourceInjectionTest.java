package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Flow451Result;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.MultiInjectionFlow;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.SimpleInjectionFlow;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.StaticResourcesFlow;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.ProducerDerived451Job;
import ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs.Resource451DoubleJob;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.resource451;

public class ResourceInjectionTest extends FlowEngineTestBase {

    @Test
    public void simpleInjection() {
        flowTester.register(SimpleInjectionFlow.getFlow(), SimpleInjectionFlow.FLOW_NAME);

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleInjectionFlow.FLOW_NAME);

        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId, SimpleInjectionFlow.DOUBLER_JOB_ID
        );

        Flow451Result result = flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance());

        // Producer451 +-----> Doubler451
        Assertions.assertEquals(451 * 2, result.getValue());
    }

    @Test
    public void simpleWithStaticInjection() {
        var value = 34;
        flowTester.register(
                SimpleInjectionFlow.getFlow(resource451(value)),
                SimpleInjectionFlow.FLOW_NAME
        );

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(SimpleInjectionFlow.FLOW_NAME);

        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId, SimpleInjectionFlow.DOUBLER_JOB_ID
        );

        Flow451Result result = flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance());

        // Upstream resource will be ignored, static resource will be used instead
        Assertions.assertEquals(value * 2, result.getValue());
    }

    @Test
    public void simpleDerivedInjection() {
        final String jobId = "Resource451DoubleJob";

        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(Resource451DoubleJob.ID, jobId)
                .withUpstreams(builder.withJob(ProducerDerived451Job.ID, "j1"));

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(builder.build());
        StoredResourceContainer producedResources = flowTester.getProducedResources(flowLaunchId, jobId);

        // ProducerDerived451Job +-----> Doubler451
        Assertions.assertEquals(
                451 * 2,
                flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance()).getValue()
        );
    }

    @Test
    public void staticDerivedInjection() {
        final String jobId = "Resource451DoubleJob";

        var value = 35;

        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(Resource451DoubleJob.ID, jobId)
                .withResources(resource451(value));

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(builder.build());
        StoredResourceContainer producedResources = flowTester.getProducedResources(flowLaunchId, jobId);

        Assertions.assertEquals(value * 2,
                flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance()).getValue());
    }

    @Test
    public void multiDerivedInjection() {
        flowTester.register(MultiInjectionFlow.getFlow(), MultiInjectionFlow.FLOW_NAME);

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(MultiInjectionFlow.FLOW_NAME);

        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId, MultiInjectionFlow.SUMMATOR_JOB_ID
        );

        Flow451Result result = flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance());

        /*
             +-->ProducerNested451Job+--+
             |                          v
        Producer451                 Summator451
             |                          ^
             +-->Producer451+-----------+
         */

        Assertions.assertEquals(451 * 3, result.getValue());
    }

    @Test
    public void multiDerivedWithStaticInjection() {
        var value = 36;
        flowTester.register(
                MultiInjectionFlow.getFlow(resource451(value)),
                MultiInjectionFlow.FLOW_NAME
        );

        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(MultiInjectionFlow.FLOW_NAME);

        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId, MultiInjectionFlow.SUMMATOR_JOB_ID
        );

        Flow451Result result = flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance());
        Assertions.assertEquals(value, result.getValue());
    }

    @Test
    public void staticResourceInjection() {
        flowTester.register(StaticResourcesFlow.getFlow(), StaticResourcesFlow.FLOW_NAME);
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(StaticResourcesFlow.FLOW_NAME);

        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId, StaticResourcesFlow.DOUBLER_JOB_ID
        );

        Flow451Result result = flowTester.getResourceOfType(producedResources, Flow451Result.getDefaultInstance());

        // Doubler451
        Assertions.assertEquals(451 * 2, result.getValue());
    }
}
