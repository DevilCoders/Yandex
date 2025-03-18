package ru.yandex.ci.flow.engine.definition;

import java.util.UUID;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

public class JobBuilderTest {
    @ExecutorInfo
    public static class TestJob implements JobExecutor {

        public static final UUID ID = UUID.fromString("07096dff-25b1-4b2a-a5eb-f90dbc3f1994");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
        }
    }

    @Test
    public void usesDefaults() {
        JobBuilder builder = FlowBuilder.create().withJob(TestJob.ID, "j1");
        Assertions.assertNull(builder.getRetry());
    }
}
