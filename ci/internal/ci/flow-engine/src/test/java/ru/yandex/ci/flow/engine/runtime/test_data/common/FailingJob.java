package ru.yandex.ci.flow.engine.runtime.test_data.common;

import java.util.UUID;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

public class FailingJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("c8d782e3-616d-4ec9-90de-22666200ebb7");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        throw new RuntimeException("Ouch!");
    }
}
