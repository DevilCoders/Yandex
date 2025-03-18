package ru.yandex.ci.flow.engine.runtime.test_data.resource_injection.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.resource451;

@Produces(single = {Resource451.class})
public class Producer451Job implements JobExecutor {

    public static final UUID ID = UUID.fromString("b779b990-15db-415f-8080-15117972a0af");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.resources().produce(resource451(451));
    }
}
