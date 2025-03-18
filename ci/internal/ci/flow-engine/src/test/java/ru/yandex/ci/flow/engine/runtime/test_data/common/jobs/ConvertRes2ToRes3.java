package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.core.test.resources.Res3;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res3;

@Consume(name = "res2", proto = Res2.class)
@Produces(single = Res3.class)
public class ConvertRes2ToRes3 implements JobExecutor {
    public static final UUID ID = UUID.fromString("1e922fc5-dc27-477c-8d8f-a0b063047e75");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var res2 = context.resources().consume(Res2.class);
        context.resources().produce(res3(res2.getS() + " " + getClass().getSimpleName()));
    }
}
