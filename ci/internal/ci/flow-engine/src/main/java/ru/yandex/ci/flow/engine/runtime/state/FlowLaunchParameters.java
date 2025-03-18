package ru.yandex.ci.flow.engine.runtime.state;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchRef;

@Value
@Builder
public class FlowLaunchParameters implements FlowLaunchRef {

    @Nonnull
    FlowLaunchId flowLaunchId;
    @Nonnull
    LaunchParameters launchParameters;

    @Override
    public FlowFullId getFlowFullId() {
        return launchParameters.getFlowInfo().getFlowId();
    }

    @Nonnull
    @Override
    public FlowLaunchId getFlowLaunchId() {
        return flowLaunchId;
    }

}
