package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.test_data.autowired_job.AutowiredFlow;

public class JobAutowiredFieldsTest extends FlowEngineTestBase {

    @Test
    public void simpleInjection() {
        flowTester.register(AutowiredFlow.flow(), AutowiredFlow.FLOW_ID);
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(AutowiredFlow.FLOW_ID);

        StoredResourceContainer producedResources = flowTester.getProducedResources(flowLaunchId, AutowiredFlow.JOB_ID);

        Resource451 result = flowTester.getResourceOfType(producedResources, Resource451.getDefaultInstance());
        Assertions.assertEquals(451, result.getValue());
    }
}
