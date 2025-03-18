package ru.yandex.ci.flow.engine.runtime.state.calculator;

import com.google.common.annotations.VisibleForTesting;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

@RequiredArgsConstructor
public class FlowResourcesAccessor {

    private final FlowStateCalculator calculator;

    @VisibleForTesting
    public ResourceRefContainer collectJobResources(FlowLaunchEntity flowLaunch, String jobId) {
        var runtime = calculator.buildFlowLaunchRuntime(flowLaunch, null);
        var job = runtime.getJob(jobId);
        return job.collectResources();
    }
}
