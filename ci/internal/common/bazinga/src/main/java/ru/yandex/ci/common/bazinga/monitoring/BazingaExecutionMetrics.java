package ru.yandex.ci.common.bazinga.monitoring;

import java.util.List;
import java.util.concurrent.TimeUnit;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.Tag;

import ru.yandex.bolts.collection.ListF;
import ru.yandex.commune.bazinga.impl.worker.TaskExecutorMetrics;
import ru.yandex.misc.monica.core.name.MetricName;
import ru.yandex.misc.monica.util.measure.MeasureInfo;

public class BazingaExecutionMetrics implements TaskExecutorMetrics {
    private static final String PREFIX = "bazinga_tasks_";

    private final MeterRegistry registry;

    public BazingaExecutionMetrics(MeterRegistry registry) {
        this.registry = registry;
    }

    private String getStatus(MeasureInfo info) {
        return info.isSuccessful() ? "success" : "error";
    }

    private void incrementScheduleErrors(String taskName, String type) {
        this.registry
                .counter(PREFIX + "schedule_errors", List.of(Tag.of("task", taskName), Tag.of("type", type)))
                .increment();
    }

    private void incrementCronJobScheduleErrorsTotalCounter(String type) {
        this.registry.counter(PREFIX + "schedule_errors_all", List.of(Tag.of("type", type)))
                .increment();
    }

    @Override
    public void updateOnetimeInvocations(MeasureInfo info, ListF<MetricName> names) {
        names.forEach(name ->
                this.registry.counter(
                        PREFIX + "onetime_invocations",
                        List.of(Tag.of("metric", name.toString()), Tag.of("status", getStatus(info)))
                )
                        .increment());
    }

    @Override
    public void updateOnetimeTimeFromSchedule(long ms, MetricName name) {
        this.registry.timer(PREFIX + "onetime_time_from_schedule", List.of(Tag.of("metric", name.toString())))
                .record(ms, TimeUnit.MILLISECONDS);
    }

    @Override
    public void updateCronInvocations(MeasureInfo info, ListF<MetricName> names) {
        names.forEach(name -> {
            this.registry.counter(
                    PREFIX + "cron_invocations",
                    List.of(Tag.of("metric", name.toString()), Tag.of("status", getStatus(info)))
            )
                    .increment();

            this.registry.timer(
                    PREFIX + "cron_invocations_time",
                    List.of(Tag.of("metric", name.toString()), Tag.of("status", getStatus(info)))
            )
                    .record(info.elapsed().getMillis(), TimeUnit.MILLISECONDS);
        });

    }

    @Override
    public void incrementCronInvocationsCount(ListF<MetricName> names) {
        names.forEach(name -> {
            this.registry.counter(
                    PREFIX + "cron_invocations_count",
                    List.of(Tag.of("metric", name.toString()))
            )
                    .increment();
        });
    }

    @Override
    public void incrementOnetimeRetries(String taskName) {
        this.registry
                .counter(PREFIX + "onetime_retries", List.of(Tag.of("task", taskName)))
                .increment();
    }

    @Override
    public void incrementCronJobScheduleErrors(String taskName) {
        this.incrementScheduleErrors(taskName, "cron");
    }

    @Override
    public void incrementCronJobScheduleErrorsTotalCounter() {
        this.incrementCronJobScheduleErrorsTotalCounter("cron");
    }

    @Override
    public void incrementOnetimeJobScheduleErrors(String taskName) {
        this.incrementScheduleErrors(taskName, "onetime");
    }

    @Override
    public void incrementOnetimeJobScheduleErrorsTotalCounter() {
        this.incrementCronJobScheduleErrorsTotalCounter("onetime");
    }
}
