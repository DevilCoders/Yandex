package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Flow451Result;
import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.flow451Result;

@Consume(name = "resources", proto = Resource451.class, list = true)
@Produces(single = Flow451Result.class)
public class MultiResource451SumJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("3c119195-56ee-4894-81b6-1f0ac95f05c2");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var resources = context.resources().consumeList(Resource451.class);
        int sum = resources.stream().mapToInt(Resource451::getValue).sum();
        context.resources().produce(flow451Result(sum));
    }
}
