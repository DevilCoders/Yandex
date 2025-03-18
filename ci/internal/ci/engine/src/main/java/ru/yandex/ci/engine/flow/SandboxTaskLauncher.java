package ru.yandex.ci.engine.flow;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.client.sandbox.api.SandboxTaskGroupStatus;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.TaskRequirementsRamdisk;
import ru.yandex.ci.client.sandbox.api.TaskSemaphoreAcquire;
import ru.yandex.ci.client.sandbox.api.TaskSemaphores;
import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxConfig;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.resolver.DocumentSource;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.tasklet.Features;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.job.TaskletContext;
import ru.yandex.ci.util.gson.CiGson;

@Slf4j
@ParametersAreNonnullByDefault
@RequiredArgsConstructor
public class SandboxTaskLauncher {
    // \w[\w/.:-]{0,254}$
    private static final Pattern INVALID_CHARACTERS = Pattern.compile("[^\\w/.:-]+");
    private static final int MAX_LENGTH = 255;

    private static final String CI_TAG = "CI";
    private static final String LAUNCH_PREFIX = "LAUNCH:";
    private static final String PULL_REQUEST_PREFIX = "PR:";
    private static final String JOB_ID_PREFIX = "JOB-ID:";
    private static final String RELEASE_PREFIX = "RELEASE:";
    private static final String ACTION_PREFIX = "ACTION:";

    @Nonnull
    private final SchemaService schemaService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final SecurityAccessService securityAccessService;
    @Nonnull
    private final SandboxClientFactory sandboxClientFactory;
    @Nonnull
    private final UrlService urlService;
    @Nonnull
    private final String sandboxBaseUrl;
    @Nonnull
    private final TaskBadgeService taskBadgeService;
    @Nonnull
    private final TaskletContextProcessor taskletContextProcessor;

    public SandboxTaskService getTaskService(YavToken.Id yavTokenUid, String taskOwner) {

        YavSecret secret = securityAccessService.getYavSecret(yavTokenUid);
        String oauthToken = secret.getCiToken();

        return new SandboxTaskService(
                sandboxClientFactory.create(oauthToken),
                taskOwner,
                secret.getSecret()
        );
    }

    @Value
    private static class TaskLauncherParams {
        SandboxTaskService sandboxTaskService;
        FlowLaunchContext launchContext;
        JobResources jobResources;
    }

    public JobInstance runSandboxTask(
            SandboxExecutorContext sandboxExecutorContext,
            SandboxTaskService sandboxTaskService,
            FlowLaunchContext launchContext,
            JobResources jobResources
    ) {
        var params = new TaskLauncherParams(sandboxTaskService, launchContext, jobResources);
        return new SandboxTaskLauncherImpl(params, sandboxExecutorContext).launch();
    }

    public JobInstance runSandboxTasklet(
            TaskletExecutorContext taskletExecutorContext,
            SandboxTaskService sandboxTaskService,
            FlowLaunchContext launchContext,
            TaskletMetadata metadata,
            JobResources jobResources
    ) {
        var params = new TaskLauncherParams(sandboxTaskService, launchContext, jobResources);
        return new SandboxTaskletLauncherImpl(params, taskletExecutorContext, metadata).launch();
    }


    public void startTask(SandboxTaskService sandboxTaskService, JobInstance instance) {
        long sandboxTaskId = instance.getSandboxTaskId();
        Preconditions.checkState(sandboxTaskId > 0, "JobInstance.sandboxTaskId is not set");
        try {
            log.info("Starting task {}", sandboxTaskId);
            sandboxTaskService.start(sandboxTaskId);
            log.info("Task {} started", sandboxTaskId);
        } catch (Exception exception) {
            log.error("Can't start task {}", sandboxTaskId);
            TaskBadge taskBadge = TaskBadge.of(
                    TaskBadge.reservedTaskId("sandbox"),
                    KnownTaskModules.SANDBOX.name(),
                    makeTaskUrl(sandboxTaskId),
                    TaskBadge.TaskStatus.FAILED,
                    null,
                    null,
                    true
            );
            taskBadgeService.updateTaskBadges(instance, List.of(taskBadge));
            throw exception;
        }
    }

    String makeDescription(FlowLaunchContext launchContext,
                           String ciLaunchUrl,
                           String ciJobInLaunchUrl) {
        @Nullable
        var pullRequestUrl = launchContext.getLaunchPullRequestInfo() != null
                ? urlService.toPullRequest(launchContext.getLaunchPullRequestInfo().getPullRequestId())
                : null;
        @Nullable
        var pullRequestId = launchContext.getLaunchPullRequestInfo() != null
                ? launchContext.getLaunchPullRequestInfo().getPullRequestId()
                : null;
        var processType = launchContext.getLaunchId().getProcessId().getType().name().toLowerCase();

        var jobPrefix = "";
        if (!StringUtils.isEmpty(launchContext.getJobTitle())) {
            jobPrefix += launchContext.getJobTitle() + "<br>";
        }
        if (!StringUtils.isEmpty(launchContext.getJobDescription())) {
            jobPrefix += launchContext.getJobDescription() + "<br>";
        }

        var ciJobPart = jobPrefix +
                "CI job: %s%s%s".formatted(
                        !ciJobInLaunchUrl.isEmpty() ? "<a href=\"%s\">".formatted(ciJobInLaunchUrl) : "",
                        "%s [launchNumber=%d]".formatted(launchContext.getJobId(), launchContext.getJobLaunchNumber()),
                        !ciJobInLaunchUrl.isEmpty() ? "</a>" : ""
                );

        var ciFlowPart = "[flowId=%s, flowLaunchId=%s, type=%s]".formatted(
                launchContext.getFlowId().asString(),
                launchContext.getFlowLaunchId().asString(),
                processType
        );

        var ciLaunchPart = "<br>CI launch: %s%s%s %s".formatted(
                !ciLaunchUrl.isEmpty() ? "<a href=\"%s\">".formatted(ciLaunchUrl) : "",
                launchContext.getTitle(),
                !ciLaunchUrl.isEmpty() ? "</a>" : "",
                ciFlowPart
        );

        var revision = launchContext.getTargetRevision();
        var arcHashPart = "<a href=\"%s\">%s</a>".formatted(urlService.toArcCommit(revision), revision.getCommitId());
        var svnRevisionPart = revision.hasSvnRevision()
                ? " (<a href=\"%s\">r%s</a>)".formatted(urlService.toSvnRevision(revision), revision.getNumber())
                : "";
        var commitPart = "<br>Arcadia Commit: %s%s"
                .formatted(arcHashPart, svnRevisionPart);

        var pullRequestPart = pullRequestUrl != null
                ? "<br>Pull request: <a href=\"%s\">%s</a>".formatted(pullRequestUrl, pullRequestId)
                : "";

        return "%s%s%s%s".formatted(ciJobPart, ciLaunchPart, commitPart, pullRequestPart);
    }

    String makeTaskUrl(long taskId) {
        return sandboxBaseUrl + "/task/" + taskId;
    }

    private Optional<JobInstance> findJobInstance(JobInstance.Id id) {
        return db.currentOrReadOnly(() -> db.jobInstance().find(id));
    }

    @Nullable
    private static SandboxTaskPriority sandboxTaskPriority(
            FlowLaunchContext launchContext,
            DocumentSource documentSource
    ) {
        var directPriority = launchContext.getRuntimeConfig().getSandbox().getPriority();
        if (directPriority != null) {
            return fromTaskPriority(directPriority, documentSource);
        }

        var requirements = launchContext.getRequirements(); // From registry
        if (requirements != null) {
            var sandbox = requirements.getSandbox();
            if (sandbox != null) {
                var priority = sandbox.getPriority();
                if (priority != null) {
                    return fromTaskPriority(priority, documentSource);
                }
            }
        }
        return null;
    }

    private void save(JobInstance instance) {
        db.currentOrTx(() -> db.jobInstance().save(instance));
    }


    private static SandboxTaskPriority fromTaskPriority(TaskPriority priority, DocumentSource documentSource) {
        if (priority.getExpression() != null) {
            var priorityExpression = PropertiesSubstitutor.substituteToString(
                    priority.getExpression(),
                    documentSource,
                    "priority"
            );
            var priorities = priorityExpression.split(":", 2);
            if (priorities.length != 2) {
                throw new RuntimeException("Invalid priority expression: " + priorityExpression);
            }
            return new SandboxTaskPriority(
                    SandboxTaskPriority.PriorityClass.valueOf(priorities[0]),
                    SandboxTaskPriority.PrioritySubclass.valueOf(priorities[1])
            );
        } else {
            Preconditions.checkState(priority.getClazz() != null, "Invalid priority, class is null");
            Preconditions.checkState(priority.getSubclass() != null, "Invalid priority, subclass is null");
            return new SandboxTaskPriority(
                    SandboxTaskPriority.PriorityClass.valueOf(priority.getClazz().name()),
                    SandboxTaskPriority.PrioritySubclass.valueOf(priority.getSubclass().name()));
        }
    }

    private static List<NotificationSetting> sandboxNotificationSettings(List<SandboxNotificationConfig> settings) {
        return settings.stream()
                .map(config -> new NotificationSetting(
                        config.getTransport(),
                        config.getStatuses(),
                        config.getRecipients()
                ))
                .collect(Collectors.toList());
    }


    private static SandboxTaskRequirements sandboxTaskRequirements(
            @Nullable RequirementsConfig requirements,
            @Nullable Long resourceId,
            DocumentSource documentSource
    ) {
        SandboxTaskRequirements sandboxRequirements = new SandboxTaskRequirements();
        if (resourceId != null) {
            sandboxRequirements.setTasksResource(resourceId);
        }

        if (requirements != null) {
            if (requirements.getCores() != null) {
                sandboxRequirements.setCores(requirements.getCores().longValue());
            }
            if (requirements.getDisk() != null) {
                sandboxRequirements.setDiskSpace(requirements.getDisk().toBytes());
            }
            if (requirements.getRam() != null) {
                sandboxRequirements.setRam(requirements.getRam().toBytes());
            }
            if (requirements.getTmp() != null) {
                sandboxRequirements.setRamdrive(
                        new TaskRequirementsRamdisk().setSize(requirements.getTmp().toBytes())
                );
            }
            SandboxConfig sandbox = requirements.getSandbox();
            if (sandbox != null) {
                var tasksResource = sandbox.getTasksResource();
                if (!StringUtils.isEmpty(tasksResource)) {
                    resourceId = PropertiesSubstitutor.substituteToNumber(tasksResource,
                            documentSource, "tasks_resource");
                    sandboxRequirements.setTasksResource(resourceId);
                }

                var containerResource = sandbox.getContainerResource();
                if (!StringUtils.isEmpty(containerResource)) {
                    var containerResourceId = PropertiesSubstitutor.substituteToNumber(containerResource,
                            documentSource, "container_resource");
                    sandboxRequirements.setContainerResource(containerResourceId);
                }
                sandboxRequirements.setClientTags(sandbox.getClientTags());
                sandboxRequirements.setPortoLayers(sandbox.getPortoLayers());
                sandboxRequirements.setHost(sandbox.getHost());
                sandboxRequirements.setCpuModel(sandbox.getCpuModel());
                sandboxRequirements.setPlatform(sandbox.getPlatform());
                sandboxRequirements.setPrivileged(sandbox.getPrivileged());
                sandboxRequirements.setTcpdumpArgs(sandbox.getTcpdumpArgs());
                if (sandbox.getDns() != null) {
                    sandboxRequirements.setDns(sandbox.getDns().getRequestValue());
                }
                if (sandbox.getSemaphores() != null) {
                    var acquires = sandbox.getSemaphores().getAcquires().stream()
                            .map(acquire -> {
                                var semaphoreName = PropertiesSubstitutor.substituteToString(
                                        acquire.getName(),
                                        documentSource, "semaphore.name"
                                );
                                return new TaskSemaphoreAcquire()
                                        .setName(semaphoreName)
                                        .setCapacity(acquire.getCapacity())
                                        .setWeight(acquire.getWeight());
                            })
                            .toList();

                    sandboxRequirements.setSemaphores(new TaskSemaphores()
                            .setAcquires(acquires)
                            .setRelease(sandbox.getSemaphores().getRelease()));
                }
            }
        }
        return sandboxRequirements;
    }

    /**
     * Создает объект - который будет передан sandbox в качестве контекста задачи.
     * Может быть доступно из кода sandbox-таски
     */
    private static Map<String, Object> sandboxTaskContext(TaskletContext taskletContext, Map<String, Object> params) {
        var map = Map.<String, Object>of("__CI_CONTEXT", ProtobufSerialization.serializeToGsonMap(taskletContext));
        if (params.isEmpty()) {
            return map;
        } else {
            params.putAll(map);
            return params;
        }
    }

    private static JobInstance makeJobInstance(JobInstance.Id id) {
        return JobInstance.builder()
                .id(id)
                .runtime(TaskletRuntime.SANDBOX)
                .status(JobStatus.CREATED)
                .build();
    }

    private static Map<String, Object> extractFieldsFromResource(JsonObject jsonObject) {
        return CiGson.toMap(jsonObject);
    }

    private static List<String> substituteStrings(List<String> values, DocumentSource documentSource, String title) {
        return values.stream()
                .map(value -> PropertiesSubstitutor.substituteToString(value, documentSource, title))
                .toList();
    }

    private static List<String> makeTags(FlowLaunchContext launchContext, DocumentSource documentSource) {
        var tags = launchContext.getRuntimeConfig().getSandbox().getTags();
        var newTags = new ArrayList<String>(tags.size() + 2);
        newTags.addAll(substituteStrings(tags, documentSource, "tags"));

        var processId = getProcessId(launchContext.getLaunchId());
        var flowId = launchContext.getFlowId().asString().replace("@", "::").toUpperCase();
        var jobId = JOB_ID_PREFIX + launchContext.getJobId().toUpperCase();
        newTags.addAll(List.of(CI_TAG, flowId, processId, jobId));

        return checkTagsAndHints(newTags).stream()
                .map(String::toUpperCase)
                .toList();
    }

    private static String getProcessId(LaunchId launchId) {
        var processId = launchId.getProcessId();
        var prefix = switch (processId.getType()) {
            case RELEASE -> RELEASE_PREFIX;
            case FLOW -> ACTION_PREFIX;
            case SYSTEM -> throw new IllegalStateException();
        };
        return prefix + processId.getSubId().replace("@", "::").toUpperCase();
    }

    private static List<String> makeHints(FlowLaunchContext launchContext, DocumentSource documentSource) {
        var hints = launchContext.getRuntimeConfig().getSandbox().getHints();
        var newHints = new ArrayList<String>(hints.size() + 2);
        newHints.addAll(substituteStrings(hints, documentSource, "hints"));
        newHints.add(LAUNCH_PREFIX + shortId(launchContext.getFlowLaunchId()));

        var pullRequestInfo = launchContext.getLaunchPullRequestInfo();
        if (pullRequestInfo != null) {
            newHints.add(PULL_REQUEST_PREFIX + pullRequestInfo.getPullRequestId());
        }
        return checkTagsAndHints(newHints);
    }

    private static List<String> checkTagsAndHints(List<String> list) {
        return list.stream()
                .map(value -> INVALID_CHARACTERS.matcher(value).replaceAll("-"))
                .map(value -> StringUtils.truncate(value, MAX_LENGTH))
                .filter(StringUtils::isNotEmpty)
                .toList();
    }

    private static String shortId(FlowLaunchId flowLaunchId) {
        int maxLength = 12;
        String id = flowLaunchId.asString();
        return id.substring(0, Math.min(maxLength, id.length()));
    }

    private static List<JobResource> filter(List<JobResource> resources, JobResourceType type) {
        return resources.stream()
                .filter(resource -> resource.getResourceType().equals(type))
                .collect(Collectors.toList());
    }

    private abstract class TaskLauncher {
        final TaskLauncherParams params;

        protected TaskLauncher(TaskLauncherParams params) {
            this.params = params;
        }

        public JobInstance launch() {
            var launchContext = params.launchContext;

            var jobId = launchContext.toJobInstanceId();
            var existing = restoreJobInstance();
            if (existing.isPresent()) {
                log.info("Reusing job instance {}", jobId);
                return existing.get();
            }

            JobInstance instance = makeJobInstance(jobId);

            var taskletContext = taskletContextProcessor.getTaskletContext(
                    launchContext, params.sandboxTaskService.getSecretUid());
            var documentSource = taskletContextProcessor.getDocumentSource(
                    launchContext, taskletContext, params.jobResources.getUpstreamResources());

            var tags = makeTags(launchContext, documentSource);
            var hints = makeHints(launchContext, documentSource);

            var sandbox = launchContext.getRuntimeConfig().getSandbox();
            var runtime = SandboxTaskService.RuntimeSettings.builder()
                    .sandboxTaskPriority(sandboxTaskPriority(launchContext, documentSource))
                    .killTimeout(sandbox.getKillTimeout())
                    .notifications(sandboxNotificationSettings(sandbox.getNotifications()))
                    .requirements(sandboxTaskRequirements(launchContext.getRequirements(),
                            getSandboxResourceId(), documentSource))
                    .taskletFeatures(getFeatures())
                    .build();

            var description = makeDescription(launchContext, taskletContext.getCiUrl(), taskletContext.getCiJobUrl());

            long sandboxTaskId = createTask(documentSource, taskletContext, description, tags, hints, runtime);

            instance = instance.withSandboxTaskId(sandboxTaskId);
            save(instance);

            startTask(params.sandboxTaskService, instance);
            return instance;
        }

        private Optional<JobInstance> restoreJobInstance() {
            var launchContext = params.launchContext;
            var sandboxTaskService = params.sandboxTaskService;

            var jobId = launchContext.toJobInstanceId();
            var existingJobInstance = findJobInstance(jobId);
            if (existingJobInstance.isPresent()) { // Keep old logic
                log.info("Reusing current job instance {}, sandbox task {}",
                        jobId, existingJobInstance.get().getSandboxTaskId());
                return existingJobInstance;
            }

            if (jobId.getNumber() <= 1) {
                return Optional.empty();
            }
            var attempts = launchContext.getJobAttemptsConfig();
            if (attempts == null || attempts.getSandboxConfig() == null) {
                return Optional.empty();
            }

            if (!Objects.requireNonNullElse(attempts.getSandboxConfig().getReuseTasks(), false)) {
                return Optional.empty();
            }

            // Let's check if we have job executed before restart
            var prevJobId = jobId.withNumber(jobId.getNumber() - 1);
            var previousJobInstance = findJobInstance(prevJobId);
            if (previousJobInstance.isEmpty()) {
                log.info("Unable to find previous job instance {}", prevJobId);
                return Optional.empty();
            }

            int maxUseAttempts = Objects.requireNonNullElse(
                    attempts.getSandboxConfig().getUseAttempts(),
                    Integer.MAX_VALUE
            );

            log.info("Found previous job instance {}, sandbox task {}, max reuse {}",
                    prevJobId, previousJobInstance.get().getSandboxTaskId(), maxUseAttempts);

            // Check for Sandbox status
            var reuseJobInstance = previousJobInstance.get()
                    .withId(jobId)
                    .incReuseCount();

            if (reuseJobInstance.getReuseCount() >= maxUseAttempts) {
                log.info("Cannot reuse task no more. Max reuse count is {}, current reuse count is {}",
                        maxUseAttempts, reuseJobInstance.getReuseCount());
                return Optional.empty();
            }

            var status = sandboxTaskService.getStatus(reuseJobInstance.getSandboxTaskId());
            var taskGroup = status.getTaskGroupStatus();

            log.info("Sandbox task {} has status = {}, group = {}",
                    reuseJobInstance.getSandboxTaskId(), status, taskGroup);

            if (taskGroup == SandboxTaskGroupStatus.FINISH) { // Can't do anything with it
                return Optional.empty();
            }

            if (taskGroup == SandboxTaskGroupStatus.BREAK) { // Let's restart
                startTask(sandboxTaskService, reuseJobInstance);
            }

            db.currentOrTx(() -> db.jobInstance().save(reuseJobInstance));
            return Optional.of(reuseJobInstance);
        }

        @Nullable
        protected abstract Long getSandboxResourceId();

        protected abstract Features getFeatures();

        protected abstract long createTask(
                DocumentSource documentSource,
                TaskletContext taskletContext,
                String description,
                List<String> tags,
                List<String> hints,
                SandboxTaskService.RuntimeSettings runtime
        );
    }

    private class SandboxTaskLauncherImpl extends TaskLauncher {

        private final SandboxExecutorContext sandboxExecutorContext;

        protected SandboxTaskLauncherImpl(TaskLauncherParams params, SandboxExecutorContext sandboxExecutorContext) {
            super(params);
            this.sandboxExecutorContext = sandboxExecutorContext;
        }

        @Nullable
        @Override
        protected Long getSandboxResourceId() {
            return sandboxExecutorContext.getResourceId();
        }

        @Override
        protected Features getFeatures() {
            return Features.empty();
        }

        @Override
        protected long createTask(
                DocumentSource documentSource,
                TaskletContext taskletContext,
                String description,
                List<String> tags,
                List<String> hints,
                SandboxTaskService.RuntimeSettings runtime
        ) {
            var allResources = taskletContextProcessor.doSubstitute(params.jobResources.getResources(), documentSource);

            var customFieldsResources = filter(allResources, JobResourceType.ofSandboxTask(sandboxExecutorContext));
            var contextResources = filter(allResources, JobResourceType.ofSandboxTaskContext(sandboxExecutorContext));

            // единственный ресурс, содержащий параметры запуска, типа JobResourceType.ofSandboxTask
            Preconditions.checkArgument(customFieldsResources.size() == 1,
                    "Expected only single custom fields resource for sandbox task");
            Map<String, Object> fields = extractFieldsFromResource(customFieldsResources.get(0).getData());

            Preconditions.checkArgument(contextResources.size() <= 1,
                    "Expected no more than single context resource for sandbox task");
            Map<String, Object> context = contextResources.isEmpty() ? Map.of() :
                    extractFieldsFromResource(contextResources.get(0).getData());

            long sandboxTaskId = params.sandboxTaskService.createTask(
                    sandboxExecutorContext.getTaskType(),
                    description,
                    fields,
                    tags,
                    hints,
                    sandboxTaskContext(taskletContext, context),
                    runtime,
                    sandboxExecutorContext.getTaskClass()
            );
            log.info("Created task {} #{}", sandboxExecutorContext.getTaskType(), sandboxTaskId);

            return sandboxTaskId;
        }
    }

    private class SandboxTaskletLauncherImpl extends TaskLauncher {

        private final TaskletExecutorContext taskletExecutorContext;
        private final TaskletMetadata metadata;

        protected SandboxTaskletLauncherImpl(
                TaskLauncherParams params,
                TaskletExecutorContext taskletExecutorContext,
                TaskletMetadata metadata) {
            super(params);
            this.taskletExecutorContext = taskletExecutorContext;
            this.metadata = metadata;
        }

        @Nullable
        @Override
        protected Long getSandboxResourceId() {
            return metadata.getId().getSandboxResourceId();
        }

        @Override
        protected Features getFeatures() {
            return metadata.getFeatures();
        }

        @Override
        protected long createTask(
                DocumentSource documentSource,
                TaskletContext taskletContext,
                String description,
                List<String> tags,
                List<String> hints,
                SandboxTaskService.RuntimeSettings runtime
        ) {
            var resources = taskletContextProcessor.doSubstitute(params.jobResources.getResources(), documentSource);
            resources.add(JobResource.optional(taskletContext));

            JsonObject input = schemaService.composeInput(metadata, taskletExecutorContext.getSchemaOptions(),
                    resources);

            var metadataId = metadata.getId();
            long sandboxTaskId = params.sandboxTaskService.createTaskletTask(
                    metadata.getSandboxTask(),
                    metadataId.getImplementation(),
                    description,
                    input,
                    tags,
                    hints,
                    sandboxTaskContext(taskletContext, Map.of()),
                    runtime
            );

            log.info("Created tasklet {} with implementation {} #{}", metadata.getName(),
                    metadataId.getImplementation(),
                    sandboxTaskId
            );

            return sandboxTaskId;
        }
    }

}
