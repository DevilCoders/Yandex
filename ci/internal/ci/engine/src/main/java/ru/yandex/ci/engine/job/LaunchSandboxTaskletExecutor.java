package ru.yandex.ci.engine.job;

import java.util.List;
import java.util.UUID;
import java.util.function.Consumer;

import com.google.common.base.Preconditions;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.engine.flow.ProgressListener;
import ru.yandex.ci.engine.flow.SandboxTaskExecutor;
import ru.yandex.ci.flow.engine.definition.context.JobContext;

public class LaunchSandboxTaskletExecutor extends LaunchSandboxExecutorBase {

    public static final UUID ID = UUID.fromString("7f3d2ae1-4ad2-4cc9-8c78-e5133d101d8a");

    public LaunchSandboxTaskletExecutor(SandboxTaskExecutor sandboxTaskExecutor) {
        super(sandboxTaskExecutor);
    }

    @Override
    protected void execute(
            JobContext context,
            ProgressListener progressListener,
            Consumer<List<JobResource>> resourcesStore
    ) throws InterruptedException {

        var tasklet = context.getJobState().getExecutorContext().getTasklet();
        Preconditions.checkState(tasklet != null, "tasklet cannot be null at Tasklet task executor");

        sandboxTaskExecutor.executeSandboxTasklet(
                context.createFlowLaunchContext(),
                tasklet,
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
