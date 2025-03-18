package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

@Produces(single = Res2.class)
public class ProduceRes2 implements JobExecutor {

    public static final UUID ID = UUID.fromString("4e33ad71-ff16-419a-a3a8-dd2d07c0e7fb");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.resources().produce(res2(getClass().getSimpleName()));
    }
}
