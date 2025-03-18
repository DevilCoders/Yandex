package ru.yandex.ci.engine.event;

import java.time.Duration;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.WorkflowImplementation;

@SuppressWarnings("unused")
public class CiEventWorkflowImpl implements CiEventWorkflow, WorkflowImplementation {

    private final CiEventActivity ciEventActivity = TemporalService.createActivity(
            CiEventActivity.class,
            Duration.ofHours(6)
    );

    @Override
    public void send(CiEventPayload payload) {
        ciEventActivity.send(payload);
    }
}
