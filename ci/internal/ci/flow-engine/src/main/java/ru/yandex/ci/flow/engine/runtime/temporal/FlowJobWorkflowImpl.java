package ru.yandex.ci.flow.engine.runtime.temporal;

import java.time.Duration;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.WorkflowImplementation;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

@SuppressWarnings("unused")
public class FlowJobWorkflowImpl implements FlowJobWorkflow, WorkflowImplementation {

    private final FlowJobActivity flowJobActivity = TemporalService.createActivity(
            FlowJobActivity.class,
            Duration.ofHours(6)
    );

    @Override
    public void run(FullJobLaunchId id) {
        flowJobActivity.run(id);
    }
}
