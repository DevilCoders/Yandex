package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.resource451;

@Produces(single = {Resource451.class})
public class ProducerDerived451Job implements JobExecutor {

    public static final UUID ID = UUID.fromString("ab66ec32-1f68-4092-b812-bd37cf305bca");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.resources().produce(resource451(451));
    }
}
