package ru.yandex.ci.flow.engine.runtime.test_data.autowired_job;

import java.util.UUID;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.test.resources.Resource451;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.resource451;

@RequiredArgsConstructor
@Produces(single = Resource451.class)
public class AutowiredJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("611e0f8f-fe41-40ac-bddd-4c3b7991fd98");

    private final Bean451 bean451;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.resources().produce(resource451(bean451.getValue()));
    }
}
