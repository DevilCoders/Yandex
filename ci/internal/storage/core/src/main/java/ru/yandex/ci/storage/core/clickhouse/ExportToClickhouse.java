package ru.yandex.ci.storage.core.clickhouse;

import java.time.Duration;

import javax.annotation.Nonnull;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.TaskQueueName;

public class ExportToClickhouse extends AbstractOnetimeTask<BazingaIterationId> {
    public static final TaskQueueName EXPORT_QUEUE = new TaskQueueName("ch-export-task");

    private ClickHouseExportService service;

    public ExportToClickhouse(ClickHouseExportService service) {
        super(BazingaIterationId.class);
        this.service = service;
    }

    public ExportToClickhouse(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) throws Exception {
        this.service.process(params.getIterationId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(60);
    }

    @Override
    public TaskQueueName queueName() {
        return EXPORT_QUEUE;
    }
}
