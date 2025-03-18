package ru.yandex.ci.flow.engine.runtime.test_data.common;

import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@RequiredArgsConstructor
public class SleepyJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("d2188991-b094-4aec-98bf-051fe7bcc44a");

    private final Semaphore semaphore;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.progress().updateText("Sleeping");

        semaphore.release();
        Thread.sleep(TimeUnit.SECONDS.toMillis(3));

        throw new RuntimeException("I woke up and looking for broken tests");
    }

    @Override
    public void interrupt(JobContext context) {
        context.progress().updateText("Interrupting");
    }
}
