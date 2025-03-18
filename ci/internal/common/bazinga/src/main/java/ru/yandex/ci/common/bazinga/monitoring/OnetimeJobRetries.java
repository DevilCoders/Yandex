package ru.yandex.ci.common.bazinga.monitoring;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Stopwatch;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import org.joda.time.Duration;
import org.joda.time.Instant;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskRegistry;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;

public class OnetimeJobRetries {
    private static final Logger log = LoggerFactory.getLogger(OnetimeJobRetries.class);

    // Please make sure we handle as little statuses as possible
    private static final List<JobStatus> STATUSES = List.of(JobStatus.RUNNING, JobStatus.READY, JobStatus.FAILED);

    private final BazingaControllerApp controllerApp;
    private final YdbBazingaStorage bazingaStorage;
    private final List<TaskId> taskTypes;

    private final int maxJobAgeDays = 7;
    //    private final int allowedRetryCountPerDay = 10;
    private final int retryCountToCrit;

    private volatile long jobsWithRetryExceededTotal;
    private final Map<TaskId, Integer> jobsWithRetryExceededByTaskType = new ConcurrentHashMap<>();

    public OnetimeJobRetries(
            BazingaControllerApp controllerApp,
            YdbBazingaStorage bazingaStorage,
            WorkerTaskRegistry workerTaskRegistry,
            @Nullable MeterRegistry registry,
            int retryCountToCrit
    ) {
        this.controllerApp = controllerApp;
        this.bazingaStorage = bazingaStorage;
        this.retryCountToCrit = retryCountToCrit;

        this.taskTypes = workerTaskRegistry.getOnetimeTasks()
                .stream().map(OnetimeTask::id)
                .collect(Collectors.toList());

        createMetrics(registry);
    }

    private void createMetrics(@Nullable MeterRegistry registry) {
        if (registry == null) {
            return;
        }
        log.info("Creating metrics for {} task types: {}", taskTypes.size(), taskTypes);
        for (TaskId taskType : taskTypes) {
            jobsWithRetryExceededByTaskType.put(taskType, 0);
            Gauge.builder("bazinga_onetime_retry_exceeded", () -> jobsWithRetryExceededByTaskType.get(taskType))
                    .tag("task_type", taskType.getId())
                    .register(registry);
        }
        Gauge.builder("bazinga_onetime_retry_exceeded", () -> this.jobsWithRetryExceededTotal)
                .tag("task_type", "TOTAL")
                .register(registry);
    }

    @Scheduled(
            fixedRateString = "${bazinga.OnetimeJobRetries.runSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void run() {
        log.info("OnetimeJobRetries started");

        var stopwatch = Stopwatch.createStarted();
        if (controllerApp.getBazingaController().isMaster()) {
            updateMetrics();
        } else {
            zeroMetrics();
        }

        log.info("run took {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));
    }

    private void zeroMetrics() {
        for (TaskId taskType : taskTypes) {
            jobsWithRetryExceededByTaskType.put(taskType, 0);
        }
        jobsWithRetryExceededTotal = 0;
    }

    private void updateMetrics() {
        var jobs = findJobs();
        for (TaskId taskType : taskTypes) {
            jobsWithRetryExceededByTaskType.put(taskType, jobs.get(taskType));
        }
        jobsWithRetryExceededTotal = jobs.values().stream()
                .filter(total -> total > 0)
                .count();

    }

    @VisibleForTesting
    long getJobsWithRetryExceededTotal() {
        return jobsWithRetryExceededTotal;
    }

    @VisibleForTesting
    Map<TaskId, Integer> getJobsWithRetryExceededByTaskType() {
        return Collections.unmodifiableMap(jobsWithRetryExceededByTaskType);
    }

    private Map<TaskId, Integer> findJobs() {
        // retryCountToCrit + runningDays * allowedRetryCountPerDay;
        var from = Instant.now().minus(Duration.standardDays(maxJobAgeDays));
        var attempts = Option.of(retryCountToCrit);

        return taskTypes.stream()
                .collect(Collectors.toMap(
                        Function.identity(),
                        taskId -> STATUSES.stream()
                                .map(status -> bazingaStorage.findOnetimeJobCount(taskId, status, from, attempts))
                                .mapToInt(Integer::valueOf)
                                .sum()
                ));
    }
}
