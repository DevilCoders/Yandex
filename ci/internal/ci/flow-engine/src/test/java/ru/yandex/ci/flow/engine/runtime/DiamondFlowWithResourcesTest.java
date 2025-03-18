package ru.yandex.ci.flow.engine.runtime;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.test_data.diamond.DiamondFlowWithResources;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class DiamondFlowWithResourcesTest extends FlowEngineTestBase {

    @Test
    public void test() {
        flowTester.register(DiamondFlowWithResources.flow(), DiamondFlowWithResources.FLOW_NAME);
        FlowLaunchId flowLaunchId = flowTester.runFlowToCompletion(DiamondFlowWithResources.FLOW_NAME);
        StoredResourceContainer producedResources = flowTester.getProducedResources(
                flowLaunchId,
                DiamondFlowWithResources.END_JOB
        );
        assertEquals(
                "ProduceRes1 ConvertRes1ToRes2",
                flowTester.getResourceOfType(producedResources, Res2.getDefaultInstance()).getS()
        );
    }
}
