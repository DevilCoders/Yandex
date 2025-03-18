package ru.yandex.ci.flow.engine.runtime.test_data.common.jobs;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res0;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res0;

@Produces(multiple = Res0.class)
public class ProduceRes0 implements JobExecutor {
    public static final String TAG_MULTI = "multi";
    public static final UUID ID = UUID.fromString("aa76cd35-2adf-42dd-9c33-39640bab4f25");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.resources().produce(res0(getClass().getSimpleName()));

        if (context.getJobState().getTags().contains(TAG_MULTI)) {
            context.resources().produce(res0(getClass().getSimpleName() + "/1"));
        }
    }
}
