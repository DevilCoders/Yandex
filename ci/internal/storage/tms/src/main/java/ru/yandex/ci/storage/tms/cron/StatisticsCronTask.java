package ru.yandex.ci.storage.tms.cron;

import java.time.Duration;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatistics;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatisticsEntity;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

@Slf4j
public class StatisticsCronTask extends CronTask {
    private final CiStorageDb db;
    private final Schedule schedule;

    public StatisticsCronTask(CiStorageDb db, Duration period) {
        this.db = db;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(period.toSeconds()));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        var statistics = new ArrayList<StorageStatisticsEntity>();
        var stopwatch = Stopwatch.createStarted();

        // separate transactions to avoid transaction lock invalidated.
        this.db.currentOrReadOnly(() -> {
            statistics.add(
                    new StorageStatisticsEntity(StorageStatistics.ACTIVE_CHECKS, this.db.checks().countActive())
            );
        });

        this.db.currentOrReadOnly(() -> {
            statistics.add(
                    new StorageStatisticsEntity(
                            StorageStatistics.ACTIVE_ITERATIONS, this.db.checkIterations().countActive()
                    )
            );
        });

        this.db.currentOrReadOnly(() -> {
            statistics.add(
                    new StorageStatisticsEntity(
                            StorageStatistics.ACTIVE_LEFT_TASKS, this.db.checkTasks().countActive(false)
                    )
            );
        });

        this.db.currentOrReadOnly(() -> {
            statistics.add(
                    new StorageStatisticsEntity(
                            StorageStatistics.ACTIVE_RIGHT_TASKS, this.db.checkTasks().countActive(true)
                    )
            );
        });

        log.info("search took {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));

        this.db.currentOrTx(() -> this.db.storageStatistics().save(statistics));

        log.info("Statistics saved");
    }
}
