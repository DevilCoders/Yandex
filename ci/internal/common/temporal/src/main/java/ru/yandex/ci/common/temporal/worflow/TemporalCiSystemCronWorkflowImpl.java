package ru.yandex.ci.common.temporal.worflow;

import java.time.Duration;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.WorkflowCronImplementation;

public class TemporalCiSystemCronWorkflowImpl implements TemporalCiSystemCronWorkflow, WorkflowCronImplementation {

    private final TemporalLostWorkflowActivity lostWorkflowActivity = TemporalService.createActivity(
            TemporalLostWorkflowActivity.class,
            Duration.ofMinutes(5)
    );

    @Override
    public void run() {
        lostWorkflowActivity.launchLostWorkflows();
    }
}
