package ru.yandex.ci.storage.core.check.tasks;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.check.TestRestartService;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class RestartTestsTask extends AbstractOnetimeTask<BazingaIterationId> {
    private TestRestartService testRestartService;

    public RestartTestsTask(TestRestartService testRestartService) {
        super(BazingaIterationId.class);
        this.testRestartService = testRestartService;
    }

    public RestartTestsTask(@Nonnull CheckIterationEntity.Id iterationId) {
        super(new BazingaIterationId(iterationId));
    }

    @Override
    protected void execute(BazingaIterationId params, ExecutionContext context) {
        var iterationId = params.getIterationId();
        log.info("Restart iteration id {}", iterationId);

        testRestartService.run(iterationId);
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

}
