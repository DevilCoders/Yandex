package ru.yandex.ci.common.temporal.config;

import java.util.List;

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import io.temporal.worker.WorkerFactory;
import io.temporal.worker.WorkerOptions;
import io.temporal.worker.WorkflowImplementationOptions;

@CanIgnoreReturnValue
public class TemporalWorkerBuilder {
    static final int MAX_WORKFLOW_THREADS = 50;
    static final int MAX_ACTIVITY_THREADS = 200;

    static final int MAX_CRON_WORKFLOW_THREADS = 5;
    static final int MAX_CRON_ACTIVITY_THREADS = 20;

    private final String queue;
    private int maxWorkflowThreads;
    private int maxActivityThreads;
    private Class<?>[] workflowImplementationTypes = {};
    private Object[] activityImplementations = {};

    TemporalWorkerBuilder(String queue) {
        this.queue = queue;
    }

    String getQueue() {
        return queue;
    }

    public TemporalWorkerBuilder maxWorkflowThreads(int maxWorkflowThreads) {
        this.maxWorkflowThreads = maxWorkflowThreads;
        return this;
    }

    public TemporalWorkerBuilder maxActivityThreads(int maxActivityThreads) {
        this.maxActivityThreads = maxActivityThreads;
        return this;
    }

    public TemporalWorkerBuilder workflowImplementationTypes(Class<?>... workflowImplementationTypes) {
        this.workflowImplementationTypes = workflowImplementationTypes;
        return this;
    }

    public TemporalWorkerBuilder workflowImplementationTypes(List<Class<?>> workflowImplementationTypes) {
        return workflowImplementationTypes(workflowImplementationTypes.toArray(new Class<?>[]{}));
    }

    public TemporalWorkerBuilder activitiesImplementations(Object... activityImplementations) {
        this.activityImplementations = activityImplementations;
        return this;
    }

    public TemporalWorkerBuilder activitiesImplementations(List<Object> activityImplementations) {
        return activitiesImplementations(activityImplementations.toArray());
    }

    void createWorker(WorkerFactory workerFactory) {
        var options = WorkerOptions.newBuilder()
                .setMaxConcurrentWorkflowTaskExecutionSize(maxWorkflowThreads)
                .setMaxConcurrentActivityExecutionSize(maxActivityThreads)
                .validateAndBuildWithDefaults();

        var workflowImplementationOptions = WorkflowImplementationOptions.newBuilder()
                .setFailWorkflowExceptionTypes(Exception.class)
                .build();

        var worker = workerFactory.newWorker(queue, options);
        worker.registerWorkflowImplementationTypes(workflowImplementationOptions, workflowImplementationTypes);
        worker.registerActivitiesImplementations(activityImplementations);
    }

    static TemporalWorkerBuilder create(String queue) {
        return new TemporalWorkerBuilder(queue);
    }
}
