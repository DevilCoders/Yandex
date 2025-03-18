package ru.yandex.ci.engine.common;


import java.time.Duration;

import javax.annotation.Nullable;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.flow.engine.runtime.bazinga.AbstractMutexCronTask;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

public abstract class CiEngineCronTask extends AbstractMutexCronTask {

    private final Duration runDelay;
    private final Duration timeout;

    protected CiEngineCronTask(
            Duration runDelay,
            Duration timeout,
            @Nullable CuratorFramework curator
    ) {
        super(curator);
        this.runDelay = runDelay;
        this.timeout = timeout;
    }

    public Duration getRunDelay() {
        return runDelay;
    }

    public Duration getTimeout() {
        return timeout;
    }

    @Override
    public Schedule cronExpression() {
        return createScheduleDelay(getRunDelay());
    }

    @Override
    public final org.joda.time.Duration timeout() {
        return org.joda.time.Duration.millis(getTimeout().toMillis());
    }

    public static ScheduleDelay createScheduleDelay(Duration duration) {
        return new ScheduleDelay(org.joda.time.Duration.millis(duration.toMillis()));
    }

}
