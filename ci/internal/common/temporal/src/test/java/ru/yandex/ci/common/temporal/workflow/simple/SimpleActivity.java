package ru.yandex.ci.common.temporal.workflow.simple;

import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

@ActivityInterface
public interface SimpleActivity {
    @ActivityMethod
    void runActivity();
}
