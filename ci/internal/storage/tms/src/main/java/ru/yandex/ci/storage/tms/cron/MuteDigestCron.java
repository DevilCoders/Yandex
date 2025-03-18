package ru.yandex.ci.storage.tms.cron;

import java.time.Duration;

import ru.yandex.ci.storage.tms.services.MuteDigestService;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

public class MuteDigestCron extends CronTask {
    private final MuteDigestService muteDigestService;
    private final Schedule schedule;
    private final int batchSize;

    public MuteDigestCron(MuteDigestService muteDigestService, Duration periodSeconds, int batchSize) {
        this.muteDigestService = muteDigestService;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds.toSeconds()));
        this.batchSize = batchSize;
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        this.muteDigestService.execute(batchSize);
    }
}
