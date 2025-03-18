package ru.yandex.ci.engine.job;

import java.util.List;
import java.util.function.Consumer;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.engine.flow.ProgressListener;
import ru.yandex.ci.engine.flow.SandboxTaskExecutor;
import ru.yandex.ci.engine.flow.TaskProgressEvent;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

public abstract class LaunchSandboxExecutorBase implements JobExecutor {
    protected final SandboxTaskExecutor sandboxTaskExecutor;

    protected LaunchSandboxExecutorBase(SandboxTaskExecutor sandboxTaskExecutor) {
        this.sandboxTaskExecutor = sandboxTaskExecutor;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        execute(
                context,
                event -> context.progress().updateTaskState(createTaskState(event)),
                resources -> {
                    var contextResources = context.resources();
                    resources.stream()
                            .map(Resource::of)
                            .forEach(contextResources::produce);
                }
        );
    }

    private static TaskBadge createTaskState(TaskProgressEvent event) {
        return TaskBadge.of(
                TaskBadge.reservedTaskId(event.getTaskModules().name().toLowerCase()),
                event.getTaskModules().name(),
                event.getUrl(),
                toTaskStatus(event.getJobStatus()),
                null,
                null,
                true
        );
    }

    private static TaskBadge.TaskStatus toTaskStatus(JobStatus jobStatus) {
        return switch (jobStatus) {
            case FAILED -> TaskBadge.TaskStatus.FAILED;
            case SUCCESS -> TaskBadge.TaskStatus.SUCCESSFUL;
            case CREATED, RUNNING -> TaskBadge.TaskStatus.RUNNING;
        };
    }

    protected abstract void execute(
            JobContext context,
            ProgressListener progressListener,
            Consumer<List<JobResource>> resourcesStore
    ) throws InterruptedException;

    @Override
    public void interrupt(JobContext context) {
        sandboxTaskExecutor.interruptTask(context.createFlowLaunchContext());
    }
}
