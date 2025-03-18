package ru.yandex.ci.storage.core.large;

import java.time.Duration;

import javax.annotation.Nonnull;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class LargeAutostartBootstrapTask extends AbstractOnetimeTask<BazingaIterationId> {
    private LargeAutostartService largeAutostartService;

    public LargeAutostartBootstrapTask(@Nonnull LargeAutostartService largeAutostartService) {
        super(BazingaIterationId.class);
        this.largeAutostartService = largeAutostartService;
    }

    public LargeAutostartBootstrapTask(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) {
        largeAutostartService.tryAutostart(params.getIterationId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(15);
    }

}
