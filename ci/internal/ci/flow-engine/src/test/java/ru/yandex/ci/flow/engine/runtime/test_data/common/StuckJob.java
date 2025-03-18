package ru.yandex.ci.flow.engine.runtime.test_data.common;

import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@RequiredArgsConstructor
public class StuckJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("8c9ed3b1-5885-4489-80de-a70a199b406c");

    private final Semaphore semaphore;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.progress().updateText("Sleeping");
        semaphore.release();

        Thread.sleep(TimeUnit.SECONDS.toMillis(10));

        throw new RuntimeException("I woke up and looking for broken tests");
    }

    @Override
    public void interrupt(JobContext context) {
        context.progress().updateText("Interrupting");
        semaphore.release();
    }
}
