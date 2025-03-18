package ru.yandex.ci.storage.core.yt.impl;

import java.time.Duration;
import java.util.concurrent.ExecutionException;

import javax.annotation.Nonnull;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.ci.storage.core.yt.IterationToYtExporter;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.TaskQueueName;
import ru.yandex.commune.bazinga.scheduler.schedule.CompoundReschedulePolicy;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleConstant;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleLinear;
import ru.yandex.commune.bazinga.scheduler.schedule.ReschedulePolicy;

@Slf4j
public class YtExportTask extends AbstractOnetimeTask<BazingaIterationId> {
    public static final TaskQueueName EXPORT_QUEUE = new TaskQueueName("export-task");

    private static final CompoundReschedulePolicy RESCHEDULE_POLICY = new CompoundReschedulePolicy(
            new RescheduleLinear(org.joda.time.Duration.standardMinutes(1), 10),
            new RescheduleConstant(org.joda.time.Duration.standardMinutes(5), 1000)
    );

    private IterationToYtExporter exporter;

    public YtExportTask(IterationToYtExporter exporter) {
        super(BazingaIterationId.class);
        this.exporter = exporter;
    }

    public YtExportTask(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) throws ExecutionException,
            InterruptedException {
        var iterationId = params.getIterationId();
        log.info("Exporting iteration {}", iterationId);

        exporter.export(iterationId);

        log.info("Export completed");
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofHours(1);
    }

    @Override
    public ReschedulePolicy reschedulePolicy() {
        return RESCHEDULE_POLICY;
    }

    @Override
    public TaskQueueName queueName() {
        return EXPORT_QUEUE;
    }

    @Override
    public int priority() {
        return -10;
    }
}
