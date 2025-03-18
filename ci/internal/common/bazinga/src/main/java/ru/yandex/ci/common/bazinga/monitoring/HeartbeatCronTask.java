package ru.yandex.ci.common.bazinga.monitoring;

import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import org.joda.time.Duration;

import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;

@AllArgsConstructor
public class HeartbeatCronTask extends CronTask {
    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;
    private final java.time.Duration taskInterval;

    @Override
    public Schedule cronExpression() {
        return new SchedulePeriodic(taskInterval.toSeconds(), TimeUnit.SECONDS);
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        bazingaTaskManager.schedule(new HeartbeatOneTimeTask());
    }

    @Override
    public Duration timeout() {
        return Duration.standardSeconds(taskInterval.toSeconds() + 5);
    }
}
