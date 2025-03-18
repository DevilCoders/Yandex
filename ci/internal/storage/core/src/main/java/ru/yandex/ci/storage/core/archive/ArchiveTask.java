package ru.yandex.ci.storage.core.archive;

import java.time.Duration;

import lombok.Value;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.TaskQueueName;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleConstant;
import ru.yandex.commune.bazinga.scheduler.schedule.ReschedulePolicy;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class ArchiveTask extends AbstractOnetimeTask<ArchiveTask.Params> {
    public static final TaskQueueName ARCHIVE_QUEUE = new TaskQueueName("archive-task");

    private CheckArchiveService archiveService;

    public ArchiveTask(ArchiveTask.Params params) {
        super(params);
    }

    public ArchiveTask(CheckArchiveService archiveService) {
        super(Params.class);
        this.archiveService = archiveService;
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        archiveService.archive(CheckEntity.Id.of(params.checkId));
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(10);
    }

    @Override
    public ReschedulePolicy reschedulePolicy() {
        return new RescheduleConstant(
                org.joda.time.Duration.standardSeconds(10),
                Integer.MAX_VALUE
        );
    }

    @Override
    public TaskQueueName queueName() {
        return ARCHIVE_QUEUE;
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        Long checkId;
    }
}
