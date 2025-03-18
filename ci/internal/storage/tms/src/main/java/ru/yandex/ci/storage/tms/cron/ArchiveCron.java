package ru.yandex.ci.storage.tms.cron;

import ru.yandex.ci.storage.core.archive.CheckArchiveService;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

public class ArchiveCron extends CronTask {

    private final CheckArchiveService archiveService;
    private final Schedule schedule;

    public ArchiveCron(CheckArchiveService archiveService, int periodSeconds) {
        this.archiveService = archiveService;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        archiveService.planChecksToArchive();
    }
}

