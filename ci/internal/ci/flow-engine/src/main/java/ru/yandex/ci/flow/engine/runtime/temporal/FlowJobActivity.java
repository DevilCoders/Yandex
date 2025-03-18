package ru.yandex.ci.flow.engine.runtime.temporal;


import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;

@ActivityInterface
public interface FlowJobActivity {

    @ActivityMethod
    void run(FullJobLaunchId parameters);
}
