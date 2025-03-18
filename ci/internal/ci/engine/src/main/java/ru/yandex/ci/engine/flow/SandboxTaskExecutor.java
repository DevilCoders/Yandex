package ru.yandex.ci.engine.flow;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

@Slf4j
@RequiredArgsConstructor
public class SandboxTaskExecutor {

    @Nonnull
    private final CiDb db;

    @Nonnull
    private final SandboxTaskLauncher sandboxTaskLauncher;

    @Nonnull
    private final TaskletMetadataService taskletMetadataService;

    @Nonnull
    private final SandboxTaskPollerSettings sandboxTaskPollerSettings;

    @Nonnull
    private final TaskBadgeService taskBadgeService;

    public void executeSandboxTask(
            FlowLaunchContext launchContext,
            SandboxExecutorContext executorContext,
            JobResources jobResources,
            ProgressListener progressListener,
            Consumer<List<JobResource>> resourcesStore
    ) throws InterruptedException {
        log.info("Executing Sandbox Task, project {}, context {}", launchContext.getProjectId(), executorContext);

        var params = new ExecutorParams(launchContext, jobResources, progressListener, resourcesStore);
        new SandboxTaskExecutorImpl(params, executorContext).execute();
    }

    public void executeSandboxTasklet(
            FlowLaunchContext launchContext,
            TaskletExecutorContext executorContext,
            JobResources jobResources,
            ProgressListener progressListener,
            Consumer<List<JobResource>> resourcesStore
    ) throws InterruptedException {
        log.info("Executing Tasklet, project {}, context {}", launchContext.getProjectId(), executorContext);

        var params = new ExecutorParams(launchContext, jobResources, progressListener, resourcesStore);
        new SandboxTaskletExecutorImpl(params, executorContext).execute();
    }

    JobInstance waitTask(
            SandboxTaskService sandboxTaskService,
            JobInstance initialJobInstance,
            List<SandboxTaskBadgesConfig> badgesConfigs,
            boolean preserveTaskletOutput,
            boolean keepPollingStop
    ) throws InterruptedException {

        long sandboxTaskId = initialJobInstance.getSandboxTaskId();
        String sandboxTaskUrl = sandboxTaskLauncher.makeTaskUrl(sandboxTaskId);
        Map<String, TaskBadge> taskBadgesByIds = new HashMap<>();

        SandboxTaskPoller.TaskUpdateCallback callback = new SandboxTaskPoller.TaskUpdateCallback() {
            @Override
            public boolean onStatusUpdate(SandboxTaskStatus status) {
                if (status == SandboxTaskStatus.DRAFT) {
                    startDraftTask(sandboxTaskService, initialJobInstance);
                    return true;
                }
                JobInstance instance = updateJobInstance(status, initialJobInstance.getId(), keepPollingStop);
                return !instance.getStatus().isFinished();
            }

            @Override
            public boolean onTaskOutputUpdate(SandboxTaskOutput taskOutput) {
                updateTaskBadges(initialJobInstance, badgesConfigs, taskBadgesByIds, taskOutput, sandboxTaskUrl);
                return onStatusUpdate(taskOutput.getStatus());
            }

            @Override
            public void onQuotaExceeded() {
                //TODO CI-2371
            }

            @Override
            public void onQuotaRestored() {
                //TODO CI-2371
            }
        };

        SandboxTaskOutput taskOutput = SandboxTaskPoller.pollTask(
                sandboxTaskPollerSettings,
                sandboxTaskService.getSandboxClient(),
                sandboxTaskId,
                !badgesConfigs.isEmpty(),
                callback
        );

        return updateJobInstanceFinal(
                sandboxTaskService,
                taskOutput,
                initialJobInstance.getId(),
                badgesConfigs,
                sandboxTaskUrl,
                preserveTaskletOutput,
                keepPollingStop
        );
    }

    private void startDraftTask(SandboxTaskService sandboxTaskService, JobInstance instance) {
        log.info(
                "Starting task {}, which is stuck in draft status. It probably wasn't launch cause of restart.",
                instance.getSandboxTaskId()
        );
        sandboxTaskLauncher.startTask(sandboxTaskService, instance);
    }

    private JobInstance updateJobInstance(
            SandboxTaskStatus sandboxStatus,
            JobInstance.Id jobInstanceId,
            boolean keepPollingStop
    ) {
        JobInstance instance = getJobInstance(jobInstanceId);
        JobStatus currentStatus = jobStatusFromTaskStatus(sandboxStatus, keepPollingStop);
        if (currentStatus == instance.getStatus()) {
            return instance;
        }
        instance = instance
                .withStatus(currentStatus)
                .withSandboxTaskStatus(sandboxStatus);
        saveJobInstance(instance);
        return instance;
    }

    private JobInstance updateJobInstanceFinal(
            SandboxTaskService sandboxTaskService,
            SandboxTaskOutput taskOutput,
            JobInstance.Id jobInstanceId,
            List<SandboxTaskBadgesConfig> badgesConfigs,
            String sandboxTaskUrl,
            boolean preserveTaskletOutput,
            boolean keepPollingStop
    ) {
        JobInstance jobInstance = getJobInstance(jobInstanceId);
        JobStatus currentStatus = jobStatusFromTaskStatus(taskOutput.getStatus(), keepPollingStop);

        Preconditions.checkState(
                currentStatus.isFinished(),
                "Job not in final status: %s", currentStatus
        );

        if (jobInstance.getStatus() == JobStatus.SUCCESS ||
                (jobInstance.getStatus() == JobStatus.FAILED && preserveTaskletOutput)) {
            SandboxTaskService.TaskletResult taskletResult = sandboxTaskService.toTaskletResult(
                    taskOutput,
                    taskBadgeService,
                    badgesConfigs,
                    sandboxTaskUrl,
                    preserveTaskletOutput
            );

            if (taskletResult.isSuccess() || preserveTaskletOutput) {
                jobInstance = jobInstance.withOutput(taskletResult.getOutput());
                taskBadgeService.updateTaskBadges(jobInstance, taskletResult.getTaskBadges());
            }

            if (jobInstance.getStatus() != JobStatus.SUCCESS || !taskletResult.isSuccess()) {
                currentStatus = JobStatus.FAILED;
            }
        }

        jobInstance = jobInstance
                .withStatus(currentStatus)
                .withSandboxTaskStatus(taskOutput.getStatus());
        saveJobInstance(jobInstance);

        return jobInstance;
    }

    private void saveJobInstance(JobInstance jobInstance) {
        db.currentOrTx(() -> db.jobInstance().save(jobInstance));
    }

    private JobStatus jobStatusFromTaskStatus(SandboxTaskStatus taskStatus, boolean keepPollingStop) {
        if (taskStatus.isRunning()) {
            return JobStatus.RUNNING;
        }

        if (taskStatus == SandboxTaskStatus.STOPPED && keepPollingStop) {
            return JobStatus.RUNNING;
        }

        if (taskStatus == SandboxTaskStatus.SUCCESS) {
            return JobStatus.SUCCESS;
        }

        return JobStatus.FAILED;
    }

    public JobInstance getJobInstance(JobInstance.Id instanceId) {
        return db.currentOrReadOnly(() ->
                db.jobInstance().get(instanceId));
    }

    private void updateTaskBadges(
            JobInstance jobInstance,
            List<SandboxTaskBadgesConfig> badgesConfigs,
            Map<String, TaskBadge> taskBadgesByIds,
            SandboxTaskOutput taskOutput,
            String sandboxTaskUrl
    ) {

        List<TaskBadge> newBadges = taskBadgeService.toTaskBadges(taskOutput, badgesConfigs, sandboxTaskUrl);
        List<TaskBadge> changedBadges = getChangedTaskBadges(taskBadgesByIds, newBadges);

        if (!changedBadges.isEmpty()) {
            log.info("Changed badges for task id {}: {}", jobInstance.getSandboxTaskId(), changedBadges);
            taskBadgesByIds.putAll(
                    changedBadges.stream().collect(Collectors.toMap(TaskBadge::getId, Function.identity()))
            );
            taskBadgeService.updateTaskBadges(jobInstance, changedBadges);
        }
    }

    public boolean interruptTask(FlowLaunchContext launchContext) {
        Optional<JobInstance> load = db.currentOrReadOnly(() ->
                db.jobInstance().find(launchContext.toJobInstanceId()));
        if (load.isEmpty()) {
            log.info("No active instances found for launch {} and job {}",
                    launchContext.getLaunchId(), launchContext.toJobInstanceId());
            return false;
        }

        var sandboxTaskService = createTaskService(launchContext);
        sandboxTaskService.stop(load.get().getSandboxTaskId(), "interrupted via CI API");
        return true;
    }

    private SandboxTaskService createTaskService(FlowLaunchContext launchContext) {
        return sandboxTaskLauncher.getTaskService(
                launchContext.getYavTokenUid(),
                launchContext.getSandboxOwner()
        );
    }

    private List<TaskBadge> getChangedTaskBadges(Map<String, TaskBadge> currentTaskBadgesById,
                                                 List<TaskBadge> updatedTaskBadges) {
        return updatedTaskBadges.stream()
                .filter(b -> !currentTaskBadgesById.containsKey(b.getId())
                        || !currentTaskBadgesById.get(b.getId()).equals(b))
                .collect(Collectors.toList());
    }


    @Value
    private static class ExecutorParams {
        FlowLaunchContext launchContext;
        JobResources jobResources;
        ProgressListener progressListener;
        Consumer<List<JobResource>> resourcesStore;
    }

    private abstract class TaskExecutor {
        final ExecutorParams params;

        protected TaskExecutor(ExecutorParams params) {
            this.params = params;
        }

        public void execute() throws InterruptedException {
            var launchContext = params.launchContext;

            var sandboxTaskService = createTaskService(launchContext);
            var jobInstance = runTask(sandboxTaskService);

            var taskUrl = sandboxTaskLauncher.makeTaskUrl(jobInstance.getSandboxTaskId());
            params.progressListener.updated(TaskProgressEvent.ofSandbox(taskUrl, JobStatus.RUNNING));

            boolean outputOnFail = getOutputOnFail();
            boolean keepPollingStop = Objects.requireNonNullElse(
                    launchContext.getRuntimeConfig().getSandbox().getKeepPollingStopped(),
                    false);
            jobInstance = waitTask(
                    sandboxTaskService,
                    jobInstance,
                    getBadgesConfigs(),
                    outputOnFail,
                    keepPollingStop
            );
            params.progressListener.updated(TaskProgressEvent.ofSandbox(taskUrl, jobInstance.getStatus()));

            if (jobInstance.getStatus() == JobStatus.SUCCESS ||
                    (jobInstance.getStatus() == JobStatus.FAILED && outputOnFail)) {
                log.info("Collecting resources for task {}", jobInstance.getId());
                var resources = collectResources(sandboxTaskService, jobInstance);
                if (!resources.isEmpty()) {
                    log.info("Total resources: {}", resources.size());
                    params.resourcesStore.accept(resources);
                }
            }

            if (jobInstance.getStatus() == JobStatus.FAILED) {
                log.info("Task {} failed", jobInstance.getId());
                throw TaskletExecutionException.of(jobInstance);
            }

            log.info("Task {} is complete", jobInstance.getId());
        }

        private boolean getOutputOnFail() {
            var launchContext = params.launchContext;
            var attempts = launchContext.getJobAttemptsConfig();
            if (attempts != null && attempts.getIfOutput() != null) {
                return true;
            }
            return Objects.requireNonNullElse(launchContext.getRuntimeConfig().getGetOutputOnFail(), false);
        }

        protected abstract JobInstance runTask(SandboxTaskService sandboxTaskService);

        protected abstract List<SandboxTaskBadgesConfig> getBadgesConfigs();

        protected abstract List<JobResource> collectResources(SandboxTaskService sandboxTaskService,
                                                              JobInstance jobInstance);
    }

    private class SandboxTaskExecutorImpl extends TaskExecutor {
        private final SandboxExecutorContext executorContext;

        protected SandboxTaskExecutorImpl(ExecutorParams params, SandboxExecutorContext executorContext) {
            super(params);
            this.executorContext = executorContext;
        }

        @Override
        protected JobInstance runTask(SandboxTaskService sandboxTaskService) {
            return sandboxTaskLauncher.runSandboxTask(
                    executorContext,
                    sandboxTaskService,
                    params.launchContext,
                    params.jobResources
            );
        }

        @Override
        protected List<SandboxTaskBadgesConfig> getBadgesConfigs() {
            return executorContext.getBadgesConfigs();
        }

        @Override
        protected List<JobResource> collectResources(SandboxTaskService sandboxTaskService, JobInstance jobInstance) {
            return sandboxTaskService.getTaskResources(jobInstance.getSandboxTaskId(),
                    executorContext.getAcceptResourceStates());
        }
    }

    private class SandboxTaskletExecutorImpl extends TaskExecutor {
        private final TaskletExecutorContext executorContext;
        private final TaskletMetadata taskletMetadata;

        protected SandboxTaskletExecutorImpl(ExecutorParams params, TaskletExecutorContext executorContext) {
            super(params);
            this.executorContext = executorContext;
            this.taskletMetadata = taskletMetadataService.fetchMetadata(executorContext.getTaskletKey());
        }

        @Override
        protected JobInstance runTask(SandboxTaskService sandboxTaskService) {
            return sandboxTaskLauncher.runSandboxTasklet(
                    executorContext,
                    sandboxTaskService,
                    params.launchContext,
                    taskletMetadata,
                    params.jobResources
            );
        }

        @Override
        protected List<SandboxTaskBadgesConfig> getBadgesConfigs() {
            return List.of();
        }

        @Override
        protected List<JobResource> collectResources(SandboxTaskService sandboxTaskService, JobInstance jobInstance) {
            var output = jobInstance.getOutput();
            if (output != null) {
                return taskletMetadataService.extractResources(
                        taskletMetadata,
                        executorContext.getSchemaOptions(),
                        output);
            } else {
                return List.of();
            }
        }
    }

}
