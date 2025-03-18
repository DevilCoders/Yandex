package ru.yandex.ci.engine.launch;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.UUID;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.CaseFormat;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import com.google.common.collect.Sets;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.SandboxTaskConfig;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.TaskletConfig;
import ru.yandex.ci.core.config.registry.TaskletV2Config;
import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.TaskUnrecoverableException;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.core.resolver.DocumentSource;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService.LocalCacheService;
import ru.yandex.ci.engine.utils.GraphTraverser;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.CanRunWhen;
import ru.yandex.ci.flow.engine.definition.job.JobProperties;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObjectType;
import ru.yandex.ci.util.Overrider;

@Slf4j
@AllArgsConstructor
class FlowFactoryBuilder {

    private final TaskletMetadataService taskletMetadataService;
    private final TaskletV2MetadataService taskletV2MetadataService;
    private final SchemaService schemaService;
    private final SourceCodeService sourceCodeService;

    private final Collection<JobConfig> jobs;
    private final FlowBuilder flowBuilder;
    private final JobType jobType;
    private final Map<TaskId, TaskConfig> taskConfigs;
    @Nullable
    private final StageGroup stageGroup;
    @Nullable
    private final JsonElement flowVars;

    void buildJobs() throws AYamlValidationException {
        var taskletV2LocalCache = taskletV2MetadataService.getLocalCache();
        var nodes = jobs.stream()
                .map(job -> new JobNode(job) {
                    @Override
                    protected JobBuilder buildJob(JobConfig config, List<JobBuilder> parents)
                            throws AYamlValidationException {
                        return new SingleJobBuilder(taskletV2LocalCache, config, parents).tryBuildJob();
                    }
                })
                .collect(Collectors.toList());
        GraphTraverser.traverse(nodes);
    }

    private class SingleJobBuilder {
        private final LocalCacheService taskletV2LocalCache;
        private final JobConfig jobConfig;
        private final List<JobBuilder> parents;
        private final String jobId;
        private final TaskId taskId;
        @Nullable
        private final TaskConfig taskConfig;
        @Nullable
        private final DocumentSource flowVarsDoc;
        private final Type type;
        @Nullable
        private final String resolvedVersion;

        private SingleJobBuilder(
                LocalCacheService taskletV2LocalCache,
                JobConfig jobConfig,
                List<JobBuilder> parents
        ) {
            this.taskletV2LocalCache = taskletV2LocalCache;
            this.jobConfig = jobConfig;
            this.parents = parents;

            this.jobId = jobConfig.getId();
            this.taskId = jobConfig.getTaskId();
            this.taskConfig = taskConfigs.get(taskId);
            Preconditions.checkArgument(
                    taskConfig != null || !taskId.requireTaskConfig(),
                    "not found task config %s for job %s", taskId, jobId
            );
            this.flowVarsDoc = prepareFlowVars();
            this.type = Type.of(taskConfig, taskId);
            this.resolvedVersion = resolveJobVersion();
        }

        JobBuilder tryBuildJob() throws AYamlValidationException {
            try {
                return buildJobImpl();
            } catch (TaskUnrecoverableException e) {
                log.info("build job failed: jobId {}", jobConfig.getId(), e);
                throw new AYamlValidationException(jobConfig.getId(), e.getMessage());
            }
        }

        private JobBuilder buildJobImpl() throws AYamlValidationException {
            var jobBuilder = getJobBuilder();

            configBaseJob(jobBuilder);

            var contextInput = jobConfig.getContextInput();
            if (contextInput != null && contextInput.isJsonObject()) {
                if (type == Type.SANDBOX_TASK) {
                    jobBuilder.withResources(new Resource(
                            JobResourceType.ofSandboxTaskContext(jobBuilder.getSandboxTask()),
                            contextInput.getAsJsonObject()));
                } else {
                    throw new AYamlValidationException(jobId, "Context input is prohibited for " + type);
                }
            }

            RequirementsConfig requirements = Overrider.overrideNullable(
                    taskConfig != null ? taskConfig.getRequirements() : null,
                    jobConfig.getRequirements()
            );

            if (requirements != null) {
                jobBuilder.withRequirements(validateRequirementsConfig(requirements));
            }

            var jobRuntimeConfig = Overrider.overrideNullable(
                    taskConfig != null ? taskConfig.getRuntimeConfig() : null,
                    jobRuntimeConfig()
            );
            if (jobRuntimeConfig != null) {
                jobBuilder.withJobRuntimeConfig(validateRuntimeConfig(jobRuntimeConfig));
            }

            configConditionExpression(jobBuilder);

            configMultiply(jobBuilder);

            var manual = jobConfig.getManual();
            if (manual.isEnabled()) {
                jobBuilder.withManualTrigger();

                var prompt = manual.getPrompt();
                if (prompt == null) {
                    jobBuilder.withPrompt(null);
                } else {
                    if (flowVarsDoc == null) {
                        jobBuilder.withPrompt(prompt);
                    } else if (jobBuilder.getMultiply() != null) {
                        // Необходимо проверить корректность выражения, но не подставлять ничего
                        checkExpressionIsValid("prompt", prompt);
                        jobBuilder.withPrompt(prompt);
                    } else {
                        var resolvedPrompt = PropertiesSubstitutor.substituteToString(prompt, flowVarsDoc, "prompt");
                        jobBuilder.withPrompt(resolvedPrompt);
                    }
                }
            }

            return jobBuilder;
        }

        private RuntimeConfig validateRuntimeConfig(RuntimeConfig runtimeConfig) throws AYamlValidationException {
            var sandboxConfig = runtimeConfig.getSandbox();
            var priority = sandboxConfig.getPriority();

            // TODO: Validate tags and hints?

            if (priority != null && priority.getExpression() != null) {
                checkExpressionIsValid("runtime/sandbox/priority", priority.getExpression());
            }
            return runtimeConfig;
        }


        private RequirementsConfig validateRequirementsConfig(RequirementsConfig requirementsConfig)
                throws AYamlValidationException {
            var sandboxConfig = requirementsConfig.getSandbox();
            if (sandboxConfig != null) {
                var tasksResource = sandboxConfig.getTasksResource();
                if (!StringUtils.isEmpty(tasksResource)) {
                    checkExpressionIsValid("requirements/sandbox/tasks_resource", tasksResource);
                }
                var containerResource = sandboxConfig.getContainerResource();
                if (!StringUtils.isEmpty(containerResource)) {
                    checkExpressionIsValid("requirements/sandbox/container_resource", containerResource);
                }
                var semaphores = sandboxConfig.getSemaphores();
                if (semaphores != null) {
                    for (var acquire : semaphores.getAcquires()) {
                        checkExpressionIsValid("requirements/sandbox/semaphores/acquires", acquire.getName());
                    }
                }
            }
            return requirementsConfig;
        }

        private JobBuilder getJobBuilder() throws AYamlValidationException {
            return switch (type) {
                case TASKLET -> {
                    Preconditions.checkState(taskConfig != null, "task config cannot be null");

                    var tasklet = taskConfig.getTasklet();
                    Preconditions.checkState(tasklet != null, "tasklet cannot be null for %s", type);

                    var taskletExecutorContext = TaskletExecutorContext.of(
                            getTaskletKey(tasklet),
                            SchemaOptions.builder()
                                    .singleInput(tasklet.isSingleInput())
                                    .singleOutput(tasklet.isSingleOutput())
                                    .build()
                    );

                    var builder = flowBuilder.withJob(taskletExecutorContext, jobId, jobType);

                    var metadata = taskletMetadataService.fetchMetadata(builder.getTasklet().getTaskletKey());
                    var resources = composeDefaultInput(true)
                            .stream()
                            .flatMap(data -> schemaService.resourceDataToResources(metadata, data).stream())
                            .map(Resource::of)
                            .toArray(Resource[]::new);

                    checkRequiredParametersArePresent(resources);
                    builder.withResources(resources);

                    yield builder;
                }
                case TASKLET_V2 -> {
                    Preconditions.checkState(taskConfig != null, "task config cannot be null");

                    var tasklet = taskConfig.getTaskletV2();
                    Preconditions.checkState(tasklet != null, "tasklet-v2 cannot be null for %s", type);

                    var description = getTaskletV2Description(tasklet);
                    var metadata = taskletV2LocalCache.fetchMetadata(description);

                    var taskletExecutorContext = TaskletV2ExecutorContext.of(
                            description,
                            metadata.getId(),
                            SchemaOptions.builder()
                                    .singleInput(tasklet.isSingleInput())
                                    .singleOutput(tasklet.isSingleOutput())
                                    .build()
                    );

                    var builder = flowBuilder.withJob(taskletExecutorContext, jobId, jobType);

                    var resources = composeDefaultInput(true)
                            .stream()
                            .flatMap(data -> schemaService.resourceDataToResources(metadata, data).stream())
                            .map(Resource::of)
                            .toArray(Resource[]::new);

                    checkRequiredParametersArePresent(resources);
                    builder.withResources(resources);

                    yield builder;
                }
                case SANDBOX_TASK -> {
                    Preconditions.checkState(taskConfig != null, "task config is null");

                    var sandbox = taskConfig.getSandboxTask();
                    Preconditions.checkState(sandbox != null, "sandbox cannot be null for %s", type);

                    var sandboxExecutorContext = sandboxExecutorContext(sandbox, getOptionalVersion());
                    var builder = flowBuilder.withJob(sandboxExecutorContext, jobId, jobType);

                    var resource = composeDefaultInput(false)
                            // Даже если у sandbox таски нет параметра - вставляем пустой ресурс
                            // Таска работать не будет, если этого не сделать.
                            .or(() -> Optional.of(new JsonObject()))
                            .map(data -> new Resource(JobResourceType.ofSandboxTask(sandboxExecutorContext), data))
                            .orElseThrow();
                    checkRequiredParametersArePresent(resource);
                    builder.withResources(resource);

                    yield builder;
                }
                case INTERNAL_TASK -> {
                    Preconditions.checkState(taskConfig != null, "task config is null");

                    var internal = taskConfig.getInternalTask();
                    Preconditions.checkState(internal != null, "internal cannot be null for %s", type);

                    var uuid = UUID.fromString(internal.getUuid());
                    var sourceCodeObject = sourceCodeService.lookup(uuid)
                            .map(sourceCodeService.type(SourceCodeObjectType.JOB_EXECUTOR))
                            .map(sourceCodeService.cast(JobExecutorObject.class))
                            .orElseThrow(() -> new AYamlValidationException(jobId,
                                    String.format("SourceCodeObject for internal task %s with UUID %s not found",
                                            taskId.getId(), uuid)));

                    var builder = flowBuilder.withJob(uuid, jobId, jobType);

                    var resources = composeDefaultInput(true)
                            .stream()
                            .flatMap(data -> resourceDataToResources(sourceCodeObject, data).stream())
                            .map(jobResource -> new Resource(
                                    jobResource.getResourceType(),
                                    jobResource.getData(),
                                    jobResource.getParentField()
                            ))
                            .toArray(Resource[]::new);

                    checkRequiredParametersArePresent(resources);
                    builder.withResources(resources);

                    yield builder;
                }
                case DUMMY -> flowBuilder.withJob(DummyJob.ID, jobId, jobType);
                default -> throw new UnsupportedOperationException("Unsupported task type " + type);
            };
        }

        public List<JobResource> resourceDataToResources(JobExecutorObject object, JsonObject inputData) {
            var inputResources = new ArrayList<JobResource>();
            for (ConsumedResource resource : object.getConsumedResources().values()) {
                var field = resource.getField();
                Preconditions.checkState(field != null,
                        "Internal error. Field is not configured for internal task %s", resource);

                var snakeCaseProperty = resource.getField();
                var camelCaseProperty = CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.LOWER_CAMEL, snakeCaseProperty);

                inputResources.addAll(schemaService.convertResource(
                        resource.getResource().getResourceType(),
                        resource.getDescriptor(),
                        resource.isList(),
                        snakeCaseProperty,
                        camelCaseProperty,
                        object.getClazz(),
                        inputData
                ));
            }

            return inputResources;
        }

        @Nullable
        private RuntimeConfig jobRuntimeConfig() {
            var runtimeConfig = jobConfig.getJobRuntimeConfig();
            var killTimeout = jobConfig.getKillTimeout();
            if (killTimeout != null) {
                if (runtimeConfig == null) {
                    return RuntimeConfig.ofKillTimeout(killTimeout);
                } else {
                    if (runtimeConfig.getSandbox().getKillTimeout() == null) {
                        return runtimeConfig.withSandbox(
                                runtimeConfig.getSandbox().toBuilder()
                                        .killTimeout(killTimeout)
                                        .build());
                    }
                }
            }
            return runtimeConfig;
        }

        private SandboxExecutorContext sandboxExecutorContext(
                SandboxTaskConfig sandboxTask,
                @Nullable Long resourceId) {
            var taskBadgesConfigs = sandboxTask.getBadgesConfigs();
            var acceptResourceStates = sandboxTask.getAcceptResourceStates();

            var taskClass = sandboxTask.getName() != null ? SandboxExecutorContext.SandboxTaskClass.TASK :
                    SandboxExecutorContext.SandboxTaskClass.TEMPLATE;

            return SandboxExecutorContext.of(
                    Objects.requireNonNullElse(sandboxTask.getName(), sandboxTask.getTemplate()),
                    List.copyOf(taskBadgesConfigs),
                    Set.copyOf(acceptResourceStates),
                    resourceId,
                    taskClass);
        }

        @Nullable
        private Long getOptionalVersion()
                throws AYamlValidationException {
            Preconditions.checkState(taskConfig != null, "task config cannot be null");
            if (taskConfig.getVersions().isEmpty()) {
                return null;
            }
            return getVersionLong();
        }

        private TaskletMetadata.Id getTaskletKey(TaskletConfig taskletConfig)
                throws AYamlValidationException {
            return TaskletMetadata.Id.of(
                    taskletConfig.getImplementation(),
                    TaskletRuntime.SANDBOX,
                    getVersionLong());
        }

        private TaskletV2Metadata.Description getTaskletV2Description(TaskletV2Config taskletConfig)
                throws AYamlValidationException {
            return TaskletV2Metadata.Description.of(
                    taskletConfig.getNamespace(),
                    taskletConfig.getTasklet(),
                    getVersionString());
        }

        private Optional<JsonObject> composeDefaultInput(boolean protoTypedInput) {
            JsonObject taskInput = null;
            JsonObject jobInput = null;

            if (taskConfig != null) {
                if (taskConfig.getResources() != null) {
                    Preconditions.checkArgument(
                            taskConfig.getResources().isJsonObject(),
                            "expected json object as resources for task %s, job %s. Got: %s",
                            taskId, jobConfig.getId(), taskConfig.getResources()
                    );
                    taskInput = taskConfig.getResources().getAsJsonObject();
                }
            }

            if (jobConfig.getInput() != null) {
                Preconditions.checkArgument(
                        jobConfig.getInput().isJsonObject(),
                        "expected json object as input for a job %s. Got: %s", jobConfig.getId(),
                        jobConfig.getInput()
                );
                jobInput = jobConfig.getInput().getAsJsonObject();
            }

            return Optional.ofNullable(schemaService.override(taskInput, jobInput, protoTypedInput));
        }

        private void configBaseJob(JobBuilder jobBuilder) throws AYamlValidationException {
            Optional.ofNullable(jobConfig.getTitle())
                    .map(value -> flowVarsDoc == null ? value :
                            PropertiesSubstitutor.substituteToString(value, flowVarsDoc, "title"))
                    .or(() -> Optional.ofNullable(taskConfig).map(TaskConfig::getTitle))
                    .ifPresent(jobBuilder::withTitle);

            Optional.ofNullable(jobConfig.getDescription())
                    .map(value -> flowVarsDoc == null ? value :
                            PropertiesSubstitutor.substituteToString(value, flowVarsDoc, "description"))
                    .or(() -> Optional.ofNullable(taskConfig).map(TaskConfig::getDescription))
                    .ifPresent(jobBuilder::withDescription);

            // null - не релизный процесс
            // для cleanup job не требуется stage
            if (stageGroup != null && jobType != JobType.CLEANUP) {
                if (jobConfig.getStage() == null) {
                    // All stages are configured in AYamlPostProcessor
                    throw new AYamlValidationException(jobConfig.getId(), "Job has no declared stage");
                }
                Stage stage = stageGroup.getStage(jobConfig.getStage());
                jobBuilder.beginStage(stage);
            }

            CanRunWhen canRunWhen = switch (jobConfig.getNeedsType()) {
                case ANY -> CanRunWhen.ANY_COMPLETED;
                case ALL -> CanRunWhen.ALL_COMPLETED;
                case FAIL -> CanRunWhen.ANY_FAILED;
            };
            jobBuilder.withUpstreams(canRunWhen, parents);

            var baseAttempts = taskConfig != null ? taskConfig.getAttempts() : null;
            var attempts = Overrider.overrideNullable(
                    baseAttempts,
                    jobConfig.getAttempts()
            );

            if (attempts != null) {
                jobBuilder.withRetry(attempts);
            }

            jobBuilder.withProperties(JobProperties.of(taskId, getVersionRef()));
            jobBuilder.withSkippedByMode(jobConfig.getSkippedByMode());
        }

        @Nullable
        private DocumentSource prepareFlowVars() {
            if (flowVars == null) {
                return null;
            }
            var json = new JsonObject();
            json.add(PropertiesSubstitutor.FLOW_VARS_KEY, flowVars);
            return DocumentSource.of(json);
        }

        @Nullable
        private String resolveJobVersion() {
            var version = jobConfig.getVersion();
            if (Strings.isNullOrEmpty(version)) {
                return version;
            }
            if (flowVarsDoc == null) {
                return version;
            }
            return PropertiesSubstitutor.substituteToStringOrNumber(version, flowVarsDoc, "version");
        }

        private void checkRequiredParametersArePresent(Resource... resources) throws AYamlValidationException {

            if (Type.of(taskConfig, taskId) != Type.SANDBOX_TASK) {
                return;
            }

            Preconditions.checkState(taskConfig != null);
            Preconditions.checkState(taskConfig.getSandboxTask() != null);

            if (taskConfig.getSandboxTask().getRequiredParameters().isEmpty()) {
                return;
            }

            Set<String> missingRequiredParameters = Sets.difference(
                    Set.copyOf(taskConfig.getSandboxTask().getRequiredParameters()),
                    Arrays.stream(resources)
                            .flatMap(res -> res.getData().keySet().stream())
                            .collect(Collectors.toSet())
            );

            if (!missingRequiredParameters.isEmpty()) {
                String jobErrorMessage = String.format(
                        "required parameters for task %s not found: %s",
                        taskId, missingRequiredParameters
                );
                throw new AYamlValidationException(jobId, jobErrorMessage);
            }
        }


        private void configConditionExpression(JobBuilder jobBuilder) throws AYamlValidationException {
            var runIf = jobConfig.getRunIf();
            if (StringUtils.isEmpty(runIf)) {
                return; // ---
            }

            checkExpressionIsValid("if", runIf);

            jobBuilder.withConditionalRunExpression(runIf);
        }

        private void configMultiply(JobBuilder jobBuilder)
                throws AYamlValidationException {
            var multiply = jobConfig.getMultiply();
            if (multiply == null) {
                return; // ---
            }

            boolean expectField = type == Type.TASKLET || type == Type.TASKLET_V2;
            if (!expectField && !Strings.isNullOrEmpty(multiply.getField())) {
                throw new AYamlValidationException(jobId, String.format(
                        "%s multiply config %s have \"%s\" value",
                        type, "cannot", "as-field"
                ));
            }

            String by = multiply.getBy();
            if (Strings.isNullOrEmpty(by)) {
                throw new AYamlValidationException(jobId, String.format(
                        "multiply config must have \"%s\" value",
                        "by"
                ));
            }

            checkExpressionIsValid("multiply/by", by);

            jobBuilder.withMultiply(multiply);
        }

        private TaskVersion getVersionRef() {
            return Strings.isNullOrEmpty(resolvedVersion)
                    ? TaskVersion.STABLE
                    : TaskVersion.of(resolvedVersion);
        }

        private Long getVersionLong() throws AYamlValidationException {
            return getVersion(version -> {
                try {
                    return Long.parseLong(version);
                } catch (NumberFormatException e) {
                    return null;
                }
            });
        }

        private String getVersionString() throws AYamlValidationException {
            return getVersion(Function.identity());
        }

        private <T> T getVersion(Function<String, T> parser) throws AYamlValidationException {
            Preconditions.checkState(taskConfig != null, "taskConfig cannot be null");

            var versionRef = getVersionRef();
            var version = taskConfig.getVersions().get(versionRef);

            if (version != null) {
                var parsedVersion = parser.apply(version);
                if (parsedVersion != null) {
                    return parsedVersion;
                }
            }

            var parsedVersionRef = parser.apply(versionRef.getValue());
            if (parsedVersionRef != null) {
                return parsedVersionRef;
            }

            var title = jobConfig.getTitle();
            var versions = taskConfig.getVersions().keySet().stream()
                    .map(TaskVersion::toString)
                    .collect(Collectors.joining(", "));

            throw new AYamlValidationException(jobConfig.getId(),
                    String.format(
                            "not found version '%s' for job '%s%s'. Available versions: %s",
                            versionRef,
                            jobConfig.getId(),
                            Strings.isNullOrEmpty(title) ? "" : " (" + title + ")",
                            versions
                    )
            );
        }

        private void checkExpressionIsValid(String field, String expression)
                throws AYamlValidationException {
            try {
                PropertiesSubstitutor.cleanup(new JsonPrimitive(expression));
            } catch (TaskUnrecoverableException e) {
                throw new AYamlValidationException(jobConfig.getId(),
                        "Unable to validate '" + field + "' expression: " + e.getMessage());
            }
        }

    }

    private abstract static class JobNode implements GraphTraverser.Node<JobBuilder> {
        private final JobConfig config;

        private JobNode(JobConfig config) {
            this.config = config;
        }

        @Override
        public String key() {
            return config.getId();
        }

        @Override
        public List<String> parents() {
            return config.getNeeds();
        }

        @Override
        public JobBuilder create(List<JobBuilder> parents) throws AYamlValidationException {
            return buildJob(config, parents);
        }

        protected abstract JobBuilder buildJob(JobConfig config, List<JobBuilder> parents)
                throws AYamlValidationException;
    }
}
