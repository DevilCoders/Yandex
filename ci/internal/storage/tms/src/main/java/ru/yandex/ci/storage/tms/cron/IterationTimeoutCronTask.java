package ru.yandex.ci.storage.tms.cron;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.base.Stopwatch;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.IsolationLevel;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.commune.bazinga.scheduler.CronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleDelay;

@Slf4j
public class IterationTimeoutCronTask extends CronTask {
    private final CiStorageDb db;
    private final StorageEventsProducer storageEventsProducer;
    private final Duration cancelOlderThan;
    private final Clock clock;
    private final Schedule schedule;

    public IterationTimeoutCronTask(
            CiStorageDb db,
            StorageEventsProducer storageEventsProducer,
            Clock clock,
            Duration cancelOlderThan,
            Duration periodSeconds
    ) {
        this.db = db;
        this.storageEventsProducer = storageEventsProducer;
        this.cancelOlderThan = cancelOlderThan;
        this.clock = clock;
        this.schedule = new ScheduleDelay(org.joda.time.Duration.standardSeconds(periodSeconds.toSeconds()));
    }

    @Override
    public Schedule cronExpression() {
        return this.schedule;
    }

    @Override
    public void execute(ExecutionContext executionContext) {
        for (var i = 0; i < 32; ++i) {
            var ids = this.cancel();
            if (ids.isEmpty()) {
                return;
            }

            for (var iterationId : ids) {
                storageEventsProducer.onCancelRequested(iterationId);
            }
        }
    }

    private Set<CheckIterationEntity.Id> cancel() {
        var timeoutTime = clock.instant().minus(this.cancelOlderThan);

        var stopwatch = Stopwatch.createStarted();
        var timeout = findTimeout(timeoutTime);

        log.info("Timeout search took {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));

        if (timeout.getIterations().isEmpty()) {
            return Set.of();
        }

        log.info(
                "Cancelling checks: {}, iterations: {}, tasks: {}",
                timeout.getChecks().stream().map(CheckEntity::getId).collect(Collectors.toSet()),
                timeout.getIterations().stream().map(CheckIterationEntity::getId).collect(Collectors.toSet()),
                timeout.getTasks().stream().map(CheckTaskEntity::getId).collect(Collectors.toSet())

        );
        this.db.currentOrTx(() -> {
            this.db.checks().save(
                    timeout.checks.stream()
                            .map(check -> check.withStatus(CheckStatus.CANCELLING_BY_TIMEOUT))
                            .collect(Collectors.toList())
            );
            this.db.checkIterations().save(
                    timeout.iterations.stream()
                            .map(iteration -> iteration.withStatus(CheckStatus.CANCELLING_BY_TIMEOUT))
                            .collect(Collectors.toList())
            );

            this.db.checkTasks().save(
                    timeout.tasks.stream()
                            .map(t -> t.withStatus(CheckStatus.CANCELLING_BY_TIMEOUT))
                            .collect(Collectors.toList())
            );
        });

        return timeout.iterations.stream().map(CheckIterationEntity::getId).collect(Collectors.toSet());
    }

    private TimeoutEntities findTimeout(Instant timeoutTime) {
        var iterations = this.db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> this.db.checkIterations().findRunningStartedBefore(timeoutTime, 8));

        if (iterations.isEmpty()) {
            return new TimeoutEntities(List.of(), List.of(), List.of());
        }

        var checks = this.db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() ->
                        this.db.checks().find(
                                iterations.stream().map(x -> x.getId().getCheckId()).collect(Collectors.toSet())
                        )
                );

        // todo orm in not working with index, so fetch by 1
        var tasks = new ArrayList<CheckTaskEntity>();
        this.db.scan().run(() ->
                iterations.forEach(
                        iteration -> tasks.addAll(this.db.checkTasks().getByIteration(iteration.getId()))
                )
        );

        return new TimeoutEntities(checks, iterations, tasks);
    }

    @Value
    private static class TimeoutEntities {
        List<CheckEntity> checks;
        List<CheckIterationEntity> iterations;
        List<CheckTaskEntity> tasks;
    }
}
