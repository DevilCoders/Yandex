package ru.yandex.ci.engine.job;

import java.util.List;
import java.util.UUID;
import java.util.function.Consumer;

import com.google.common.base.Preconditions;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.engine.flow.ProgressListener;
import ru.yandex.ci.engine.flow.SandboxTaskExecutor;
import ru.yandex.ci.flow.engine.definition.context.JobContext;

public class LaunchSandboxTaskExecutor extends LaunchSandboxExecutorBase {

    public static final UUID ID = UUID.fromString("a3cf3525-b29b-41f4-8343-52668c16b267");

    public LaunchSandboxTaskExecutor(SandboxTaskExecutor sandboxTaskExecutor) {
        super(sandboxTaskExecutor);
    }

    @Override
    protected void execute(
            JobContext context,
            ProgressListener progressListener,
            Consumer<List<JobResource>> resourcesStore
    ) throws InterruptedException {

        SandboxExecutorContext sandboxTask = context.getJobState().getExecutorContext().getSandboxTask();
        Preconditions.checkState(sandboxTask != null, "sandboxTask cannot be null at Sandbox task executor");

        sandboxTaskExecutor.executeSandboxTask(
                context.createFlowLaunchContext(),
                sandboxTask,
                context.resources().getJobResources(),
                progressListener,
                resourcesStore
        );
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }
}
