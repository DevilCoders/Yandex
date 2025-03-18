package ru.yandex.ci.common.temporal.worflow;

import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

@ActivityInterface
public interface TemporalLostWorkflowActivity {
    @ActivityMethod
    void launchLostWorkflows();
}
