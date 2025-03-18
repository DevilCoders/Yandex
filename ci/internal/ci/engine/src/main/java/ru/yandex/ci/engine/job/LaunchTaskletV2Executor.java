package ru.yandex.ci.engine.job;

import java.time.Duration;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.UUID;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import com.google.protobuf.ByteString;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.flow.TaskletExecutionException;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.tasklet.api.v2.DataModel;
import ru.yandex.tasklet.api.v2.DataModel.Execution;
import ru.yandex.tasklet.api.v2.DataModel.ExecutionInput;
import ru.yandex.tasklet.api.v2.DataModel.ExecutionRequirements;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.ExecuteRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetExecutionRequest;

@Slf4j
@RequiredArgsConstructor
public class LaunchTaskletV2Executor implements JobExecutor {

    public static final UUID ID = UUID.fromString("700d740f-f27b-4c7c-9a50-c52d4fd78167");

    @Nonnull
    private final TaskletV2Client taskletV2Client;
    @Nonnull
    private final SecurityAccessService securityAccessService;
    @Nonnull
    private final TaskletV2MetadataService taskletV2MetadataService;
    @Nonnull
    private final TaskletContextProcessor taskletContextProcessor;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final Duration checksInterval;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        new JobExecution(context).execute();
    }

    private YavSecret getYavSecret(FlowLaunchContext flowLaunchContext) {
        return securityAccessService.getYavSecret(flowLaunchContext.getYavTokenUid());
    }

    private Optional<JobInstance> findJobInstance(JobInstance.Id id) {
        return db.currentOrReadOnly(() -> db.jobInstance().find(id));
    }

    private JobInstance saveJobInstance(JobInstance jobInstance) {
        return db.currentOrTx(() -> db.jobInstance().save(jobInstance));
    }

    private class JobExecution {
        private final TaskletV2ExecutorContext taskletExecutorContext;
        private final TaskletV2Metadata taskletMetadata;
        private final FlowLaunchContext launchContext;
        private final JobProgressContext jobProgressContext;
        private final JobResourcesContext jobResourcesContext;
        @Nullable
        private final String secretUid;
        private final TaskletV2Client.AuthenticatedExecutor executor;

        JobExecution(JobContext context) {
            this.taskletExecutorContext = Objects.requireNonNull(
                    context.getJobState().getExecutorContext().getTaskletV2(),
                    "tasklet-v2 execution context cannot be null"
            );
            this.taskletMetadata = taskletV2MetadataService.fetchMetadata(taskletExecutorContext.getTaskletKey());

            this.launchContext = context.createFlowLaunchContext();
            this.jobProgressContext = context.progress();
            this.jobResourcesContext = context.resources();

            var secret = getYavSecret(launchContext);
            this.secretUid = secret.getSecret();
            this.executor = taskletV2Client.getExecutor(secret.getCiToken());
        }

        void execute() {
            // Unlike Tasklet v1/Sandbox we don't have much options here
            onJobComplete(waitJob(registerJob()));
        }

        private void onJobComplete(JobInstance jobInstance) {
            log.info("Completing job: {}, status: {}", jobInstance.getId(), jobInstance.getStatus());
            Preconditions.checkState(
                    jobInstance.getStatus().isFinished(),
                    "Internal error, job instance %s is not finished: %s",
                    jobInstance.getId(), jobInstance.getStatus()
            );

            boolean failed = jobInstance.getStatus() == JobStatus.FAILED;
            boolean updateOutput;
            if (failed) {
                updateOutput = Objects.requireNonNullElse(
                        launchContext.getRuntimeConfig().getGetOutputOnFail(),
                        false);
            } else {
                updateOutput = true;
            }

            if (updateOutput) {
                var resources = collectResources(jobInstance);
                log.info("Total output resources: {}", resources.size());

                resources.stream()
                        .map(Resource::of)
                        .forEach(jobResourcesContext::produce);
            }

            if (failed) {
                log.info("Job {} failed", jobInstance.getId());
                throw TaskletExecutionException.of(jobInstance);
            } else {
                log.info("Job {} success", jobInstance.getId());
            }
        }

        private JobInstance waitJob(JobInstance jobInstance) {
            log.info("Waiting for job to complete: {}", jobInstance.getId());

            // TODO: temporary solution until status stream implementation
            while (!jobInstance.getStatus().isFinished()) {
                jobInstance = getJobState(jobInstance);
                log.info("Sleep {}...", checksInterval);
                try {
                    //noinspection BusyWait
                    Thread.sleep(checksInterval.toMillis());
                } catch (InterruptedException e) {
                    throw new RuntimeException("Thread sleep is interrupted", e);
                }
            }

            return jobInstance;
        }

        private JobInstance registerJob() {
            var currentInstance = restoreJobInstance();
            if (currentInstance.isPresent()) {
                log.info("Accept existing job instance: {}", currentInstance.get());
                return startJobIfRequired(currentInstance.get());
            }

            var id = launchContext.toJobInstanceId();
            var newInstance = JobInstance.builder()
                    .id(id)
                    .runtime(TaskletRuntime.TASKLET_V2)
                    .status(JobStatus.CREATED)
                    .build();

            log.info("Registering new job instance: {}", newInstance.getId());
            return startJobIfRequired(saveJobInstance(newInstance));
        }

        private JobInstance startJobIfRequired(JobInstance jobInstance) {
            if (jobInstance.getStatus() != JobStatus.CREATED) {
                return jobInstance;
            }

            log.info("Starting job: {}", jobInstance.getId());

            var description = taskletExecutorContext.getTaskletDescription(); // TODO: use build id,
            var request = ExecuteRequest.newBuilder()
                    .setNamespace(description.getNamespace())
                    .setTasklet(description.getTasklet())
                    .setLabel(description.getLabel())
                    .setRequirements(ExecutionRequirements.newBuilder()
                            .setAccountId(launchContext.getSandboxOwner())  // TODO: to Sandbox or not to Sandbox
                    )
                    .setInput(ExecutionInput.newBuilder()
                            .setSerializedData(buildInput())
                    )
                    .build();
            var response = executor.execute(request);

            return updateJobInstance(jobInstance, response.getExecution());
        }

        private JobInstance getJobState(JobInstance jobInstance) {
            Preconditions.checkState(StringUtils.isNotEmpty(jobInstance.getExecutionId()),
                    "Internal error, Tasklet V2 execution is empty in job %s", jobInstance.getId());

            var request = GetExecutionRequest.newBuilder()
                    .setId(jobInstance.getExecutionId())
                    .build();
            var response = executor.getExecution(request);
            return updateJobInstance(jobInstance, response.getExecution());
        }

        private JobInstance updateJobInstance(JobInstance jobInstance, Execution execution) {
            var meta = execution.getMeta();
            var status = execution.getStatus();
            log.info("Execution: {}", meta);
            log.info("Status: {}", status);

            var jobStatus = toJobStatus(status);
            var update = jobInstance.toBuilder()
                    .executionId(meta.getId())
                    .taskletStatus(status.getStatus())
                    .taskletServerError(null)
                    .status(jobStatus);

            update.errorDescription(null);
            update.transientError(null);
            if (jobStatus.isFinished()) {
                update.output(buildOutput(execution));
                update.taskletServerError(buildServerError(execution));
                var error = buildError(execution);
                if (error != null) {
                    update.errorDescription(error.message);
                    update.transientError(error.transientError);
                }
            } else {
                update.output(null);
            }

            var newJobInstance = update.build();
            if (Objects.equals(newJobInstance, jobInstance)) {
                return jobInstance;
            } else {
                log.info("Saving job instance: {}", newJobInstance);
                updateTaskBadge(execution, jobStatus);
                return saveJobInstance(newJobInstance);
            }
        }

        @Nullable
        private JsonObject buildOutput(Execution execution) {
            log.info("Reading output");

            var processingResult = execution.getStatus().getProcessingResult();
            if (processingResult.hasOutput()) {
                var output = processingResult.getOutput().getSerializedOutput();
                if (output.isEmpty()) {
                    return null;
                }
                return taskletV2MetadataService.buildOutput(taskletMetadata, output);
            } else {
                return null;
            }
        }

        @Nullable
        private DataModel.ErrorCodes.ErrorCode buildServerError(Execution execution) {
            var status = execution.getStatus().getStatus();
            if (status == DataModel.EExecutionStatus.E_EXECUTION_STATUS_FINISHED) {
                var processingResult = execution.getStatus().getProcessingResult();
                if (processingResult.hasServerError()) {
                    return processingResult.getServerError().getCode();
                }
            }
            return null;
        }

        @Nullable
        private ErrorDescription buildError(Execution execution) {
            log.info("Reading error");

            var status = execution.getStatus().getStatus();
            if (status == DataModel.EExecutionStatus.E_EXECUTION_STATUS_INVALID) {
                return ErrorDescription.of("Tasklet internal error", false);
            }

            var processingResult = execution.getStatus().getProcessingResult();
            if (processingResult.hasUserError()) {
                var userError = processingResult.getUserError();
                var description = userError.getDescription();
                var message = description.isEmpty() ? null : description;
                return ErrorDescription.of(message, userError.getIsTransient());
            } else if (processingResult.hasServerError()) {
                var serverError = processingResult.getServerError();
                var message = "Server error " + serverError.getCode();
                if (!serverError.getDescription().isEmpty()) {
                    message += ", " + serverError.getDescription();
                }
                return ErrorDescription.of(message, serverError.getIsTransient());
            } else {
                return null;
            }
        }

        private ByteString buildInput() {
            log.info("Building tasklet context...");
            var taskletContext = taskletContextProcessor.getTaskletContext(launchContext, secretUid);

            var jobResources = jobResourcesContext.getJobResources();
            var documentSource = taskletContextProcessor.getDocumentSource(
                    launchContext,
                    taskletContext,
                    jobResources.getUpstreamResources()
            );

            log.info("Resolving resources ({} resources)...", jobResources.getResources().size());
            var resources = taskletContextProcessor.doSubstitute(jobResources.getResources(), documentSource);
            resources.add(JobResource.optional(taskletContext));

            log.info("Composing input ({} resources)...", resources.size());
            var input = taskletV2MetadataService.composeInput(
                    taskletMetadata,
                    taskletExecutorContext.getSchemaOptions(),
                    resources
            );

            var bytes = input.toByteString();
            log.info("Input of {}, total {} bytes", input.getDescriptorForType().getFullName(), bytes.size());
            return bytes;
        }

        private Optional<JobInstance> restoreJobInstance() {
            var jobId = launchContext.toJobInstanceId();
            var existingJobInstance = findJobInstance(jobId);
            if (existingJobInstance.isPresent()) { // Keep old logic
                var instance = existingJobInstance.get();
                if (instance.getExecutionId() != null) {
                    log.info("Reusing current job instance {}, execution id {}", jobId, instance.getExecutionId());
                    return existingJobInstance;
                }
            }

            // We can't reuse jobs in Tasklet V2
            return Optional.empty();
        }

        private List<JobResource> collectResources(JobInstance jobInstance) {
            var output = jobInstance.getOutput();
            if (output != null) {
                return taskletV2MetadataService.extractResources(
                        taskletMetadata,
                        taskletExecutorContext.getSchemaOptions(),
                        output);
            } else {
                return List.of();
            }
        }

        private JobStatus toJobStatus(DataModel.ExecutionStatus status) {
            var executionStatus = status.getStatus();
            return switch (executionStatus) {
                case E_EXECUTION_STATUS_EXECUTING -> JobStatus.RUNNING;
                case E_EXECUTION_STATUS_INVALID -> JobStatus.FAILED;
                case E_EXECUTION_STATUS_FINISHED -> {
                    var processingResult = status.getProcessingResult();
                    if (processingResult.hasOutput()) {
                        yield JobStatus.SUCCESS;
                    } else {
                        yield JobStatus.FAILED;
                    }
                }
                default -> throw new IllegalStateException("Unsupported Tasklet status: " + executionStatus);
            };
        }

        private TaskBadge.TaskStatus toBadgeStatus(JobStatus jobStatus) {
            return switch (jobStatus) {
                case CREATED, RUNNING -> TaskBadge.TaskStatus.RUNNING;
                case FAILED -> TaskBadge.TaskStatus.FAILED;
                case SUCCESS -> TaskBadge.TaskStatus.SUCCESSFUL;
            };
        }

        private void updateTaskBadge(Execution execution, JobStatus jobStatus) {
            var url = getUrl(execution.getMeta().getId());
            var badge = TaskBadge.of(
                    TaskBadge.reservedTaskId("tasklet-v2"),
                    "TASKLETV2",
                    url,
                    toBadgeStatus(jobStatus),
                    null,
                    null,
                    true
            );
            jobProgressContext.updateTaskState(badge);
        }

        private String getUrl(String executionId) {
            var desc = taskletExecutorContext.getTaskletDescription();
            var namespace = FlowUrls.encodeParameter(desc.getNamespace());
            var tasklet = FlowUrls.encodeParameter(desc.getTasklet());
            return "https://sandbox.yandex-team.ru/tasklet/%s/%s/run/%s"
                    .formatted(namespace, tasklet, executionId);
        }
    }

    @Value(staticConstructor = "of")
    private static class ErrorDescription {
        @Nullable
        String message;

        @Nullable
        Boolean transientError;
    }

}
