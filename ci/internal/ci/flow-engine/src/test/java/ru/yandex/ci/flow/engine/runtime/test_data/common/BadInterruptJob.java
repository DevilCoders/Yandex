package ru.yandex.ci.flow.engine.runtime.test_data.common;

import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@RequiredArgsConstructor
public class BadInterruptJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("10c60e63-7a6c-402a-911d-ebeb40063a82");

    private final Semaphore semaphore;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        context.progress().updateText("Sleeping");
        semaphore.release();

        Thread.sleep(TimeUnit.SECONDS.toMillis(2));
        context.progress().updateText("Woke up");

        throw new RuntimeException("I woke up and looking for broken tests");
    }

    @Override
    public void interrupt(JobContext context) {
        throw new RuntimeException("I am stuck!");
    }
}
