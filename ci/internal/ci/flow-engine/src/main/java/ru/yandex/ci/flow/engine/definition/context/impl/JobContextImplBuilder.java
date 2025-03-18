package ru.yandex.ci.flow.engine.definition.context.impl;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

@Value
@Builder(buildMethodName = "buildInternal")
public class JobContextImplBuilder {
    @Nonnull
    UpstreamResourcesCollector upstreamResourcesCollector;
    @Nonnull
    JobProgressService jobProgressService;
    @Nonnull
    FlowLaunchEntity flowLaunch;
    @Nonnull
    JobState jobState;
    @Nonnull
    FlowStateService flowStateService;
    @Nonnull
    ResourceService resourceService;
    @Nonnull
    SourceCodeService sourceCodeService;

    public static class Builder {

        public JobContextImpl build() {
            return new JobContextImpl(this.buildInternal());
        }
    }
}
