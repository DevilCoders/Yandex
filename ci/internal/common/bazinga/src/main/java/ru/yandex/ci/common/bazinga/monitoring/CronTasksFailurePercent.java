package ru.yandex.ci.common.bazinga.monitoring;

import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.controller.ControllerCronTask;
import ru.yandex.commune.bazinga.impl.controller.ControllerTask;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.scheduler.CronTaskInfo;
import ru.yandex.misc.db.q.SqlLimits;

public class CronTasksFailurePercent {
    private static final Logger log = LoggerFactory.getLogger(CronTasksFailurePercent.class);

    private final BazingaControllerApp controllerApp;
    private final BazingaStorage bazingaStorage;
    private final int failureCountToWarn;

    private volatile float failurePercent;

    public CronTasksFailurePercent(
            BazingaControllerApp controllerApp,
            BazingaStorage bazingaStorage,
            int failureCountToWarn,
            @Nullable MeterRegistry registry
    ) {
        this.controllerApp = controllerApp;
        this.bazingaStorage = bazingaStorage;
        this.failureCountToWarn = failureCountToWarn;
        if (registry != null) {
            Gauge.builder("bazinga_cron_failure_percent", () -> this.failurePercent)
                    .register(registry);
        }
    }

    public void run() {
        log.info("CronTasksFailurePercent started");

        var stopwatch = Stopwatch.createStarted();
        if (controllerApp.getBazingaController().isMaster()) {
            failurePercent = checkCronTasks();
        } else {
            failurePercent = 0;
        }

        log.info("run took {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));
    }

    public float getFailurePercent() {
        return failurePercent;
    }

    private float checkCronTasks() {
        var cronTasks = controllerApp.getTaskRegistry().getCronTasks();
        var tasks = cronTasks
                .stream()
                .filter(ControllerTask::isEnabled)
                .map(ControllerCronTask::getTaskRaw)
                .map(CronTaskInfo::getTaskId)
                .toList();

        if (tasks.size() == 0) {
            return 0f;
        }

        var failingTaskIds = tasks.stream()
                .filter(this::isFailingForTooLong)
                .toList();

        var failingCronTasksPercent = failingTaskIds.size() * 100.f / tasks.size();

        log.info(
                String.format(
                        "%d of %d (%.2f%%) cron tasks are failing (%s)",
                        failingTaskIds.size(),
                        tasks.size(),
                        failingCronTasksPercent,
                        failingTaskIds.stream().map(TaskId::toString).collect(Collectors.joining(", "))
                )
        );

        return failingCronTasksPercent;
    }

    private boolean isFailingForTooLong(TaskId taskId) {
        var failedJobs = bazingaStorage.findLatestCronJobs(taskId, SqlLimits.first(100)).stream()
                .filter(job -> job.getValue().getStatus().isCompleted())
                .filter(job -> !job.getValue().getExceptionMessage().orElse("").contains("InterruptedException"))
                .limit(failureCountToWarn)
                .toList();

        return (failedJobs.size() == failureCountToWarn) &&
                failedJobs.stream().allMatch(job -> job.getValue().getStatus() != JobStatus.COMPLETED);
    }
}
