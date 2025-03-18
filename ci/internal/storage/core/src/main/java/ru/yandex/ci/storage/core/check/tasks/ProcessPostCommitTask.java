package ru.yandex.ci.storage.core.check.tasks;

import java.time.Duration;

import javax.annotation.Nonnull;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.check.PostCommitNotificationService;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class ProcessPostCommitTask extends AbstractOnetimeTask<BazingaIterationId> {
    private PostCommitNotificationService postCommitNotificationService;

    public ProcessPostCommitTask(PostCommitNotificationService postCommitNotificationService) {
        super(BazingaIterationId.class);
        this.postCommitNotificationService = postCommitNotificationService;
    }

    public ProcessPostCommitTask(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) throws Exception {
        this.postCommitNotificationService.process(params.getIterationId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(10);
    }
}
