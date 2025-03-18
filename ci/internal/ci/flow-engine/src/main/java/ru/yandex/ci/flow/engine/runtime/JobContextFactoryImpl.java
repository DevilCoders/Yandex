package ru.yandex.ci.flow.engine.runtime;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.context.impl.JobContextImpl;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

@RequiredArgsConstructor
public class JobContextFactoryImpl implements JobContextFactory {
    @Nonnull
    private final JobProgressService jobProgressService;
    @Nonnull
    private final UpstreamResourcesCollector upstreamResourcesCollector;
    @Nonnull
    private final FlowStateService flowStateService;
    @Nonnull
    private final ResourceService resourceService;
    @Nonnull
    private final SourceCodeService sourceCodeService;

    @Override
    public JobContext createJobContext(FlowLaunchEntity flowLaunch, JobState jobState) {
        return JobContextImpl.builder()
                .flowLaunch(flowLaunch)
                .jobState(jobState)
                .jobProgressService(jobProgressService)
                .upstreamResourcesCollector(upstreamResourcesCollector)
                .flowStateService(flowStateService)
                .resourceService(resourceService)
                .sourceCodeService(sourceCodeService)
                .build();
    }
}
