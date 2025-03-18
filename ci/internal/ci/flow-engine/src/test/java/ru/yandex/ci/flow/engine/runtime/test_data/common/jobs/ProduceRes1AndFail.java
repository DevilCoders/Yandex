package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;

@Produces(multiple = Res1.class)
public class ProduceRes1AndFail implements JobExecutor {

    public static final UUID ID = UUID.fromString("661eb30b-de61-4823-8137-cf0e69535a0a");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) {
        context.resources().produce(res1("1"));
        context.resources().produce(res1("2"));
        context.resources().produce(res1("3"));
        throw new RuntimeException();
    }
}
