package ru.yandex.ci.storage.core.large;

import java.time.Duration;

import javax.annotation.Nonnull;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class LargeLogbrokerTask extends AbstractOnetimeTask<BazingaIterationId> {
    private LargeStartService largeStartService;

    public LargeLogbrokerTask(@Nonnull LargeStartService largeStartService) {
        super(BazingaIterationId.class);
        this.largeStartService = largeStartService;
    }

    public LargeLogbrokerTask(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) {
        largeStartService.sendLogbrokerEvents(params.getIterationId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

}
