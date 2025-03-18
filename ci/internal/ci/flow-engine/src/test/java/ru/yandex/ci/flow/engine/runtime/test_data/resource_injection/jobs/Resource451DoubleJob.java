package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Flow451Result;
import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.flow451Result;

@Consume(name = "resource451", proto = Resource451.class)
@Produces(single = Flow451Result.class)
public class Resource451DoubleJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("8925a387-a35f-4e5e-93d4-3f15b51aa0ad");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var resource451 = context.resources().consume(Resource451.class);
        context.resources().produce(flow451Result(resource451.getValue() * 2));
    }
}
