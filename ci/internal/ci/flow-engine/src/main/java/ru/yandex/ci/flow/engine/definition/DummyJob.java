package ru.yandex.ci.flow.engine.definition;

import java.util.UUID;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@ExecutorInfo(
    title = "Dummy",
    description = "Универсальная пустая джоба",
    recommended = true
)
public class DummyJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("3f2315b0-0a5f-42d4-b09b-6b3ddb07e77b");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
    }
}
