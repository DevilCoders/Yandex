package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import lombok.RequiredArgsConstructor;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

import static org.mockito.Mockito.verify;

public class JobExecutorConstructorWithParametersTest extends FlowEngineTestBase {

    @Autowired
    private Runnable mockRunnable;

    @Test
    public void name() {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(SomeJob.ID, "j1");
        flowTester.runFlowToCompletion(builder.build());
        verify(mockRunnable).run();
    }

    @RequiredArgsConstructor
    public static class SomeJob implements JobExecutor {
        public static final UUID ID = UUID.fromString("226678d2-e413-4285-8005-e09a6b123194");

        private final Runnable runnable;

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            runnable.run();
        }
    }
}
