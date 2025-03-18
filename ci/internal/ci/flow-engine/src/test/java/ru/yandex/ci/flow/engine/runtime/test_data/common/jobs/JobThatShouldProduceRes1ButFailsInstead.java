package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

@Produces(single = Res1.class)
public class JobThatShouldProduceRes1ButFailsInstead implements JobExecutor {

    public static final UUID ID = UUID.fromString("3138788c-703d-409f-962b-69fa91d83b67");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        throw new RuntimeException("test");
    }
}
