package ru.yandex.ci.common.temporal.workflow.cron;

import io.temporal.activity.ActivityInterface;
import io.temporal.activity.ActivityMethod;

@ActivityInterface
public interface CronTestActivity {

    @ActivityMethod
    void run();
}
