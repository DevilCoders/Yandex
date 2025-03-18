package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

public interface JobContextFactory {
    JobContext createJobContext(FlowLaunchEntity flowLaunch, JobState jobState);
}
