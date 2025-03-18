package ru.yandex.ci.storage.tms.cron;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGenerator;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

public class CheckIdGenerationCron extends CronTask {
    private final CiStorageDb db;
    private final int numberOfFreeIds;
    private final Schedule schedule;

    public CheckIdGenerationCron(CiStorageDb db, int numberOfFreeIds, int periodSeconds) {
        this.db = db;
        this.numberOfFreeIds = numberOfFreeIds;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        CheckIdGenerator.fillDb(db, numberOfFreeIds);
    }
}
