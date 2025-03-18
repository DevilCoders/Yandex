package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;

@Consume(name = "resources", proto = Res1.class, list = true)
@Produces(multiple = Res2.class)
public class ConvertMultipleRes1ToRes2 implements JobExecutor {
    public static final UUID ID = UUID.fromString("b298395c-9630-4b17-ac9b-fbf682fadff5");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var resources = context.resources().consumeList(Res1.class);
        for (int i = 0; i < resources.size(); ++i) {
            context.resources().produce(res2(resources.get(i).getS() + i));
        }
    }
}
