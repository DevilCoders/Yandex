package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

@Consume(name = "res1", proto = Res1.class)
@Produces(single = Res2.class)
public class ConvertRes1ToRes2 implements JobExecutor {
    public static final UUID ID = UUID.fromString("cde10ba4-8bc3-4be4-8d89-60771a23031a");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var res1 = context.resources().consume(Res1.class);
        context.resources().produce(res2(res1.getS() + " " + getClass().getSimpleName()));
    }
}
