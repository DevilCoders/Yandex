package ru.yandex.ci.engine;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import lombok.Builder;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskRequirements;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationSetting;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext.SandboxTaskClass;
import ru.yandex.ci.core.tasklet.Features;
import ru.yandex.ci.engine.flow.TaskBadgeService;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.tasklet.SandboxResource;
import ru.yandex.ci.util.gson.CiGson;
import ru.yandex.ci.util.gson.GsonPreciseDeserializer;

public class SandboxTaskService {
    private static final Logger log = LoggerFactory.getLogger(SandboxTaskService.class);
    private static final String DATE_TIME_FORMAT = "yyyy-MM-dd'T'HH:mm:ss.SSSSSSX";
    public static final Gson GSON = new GsonBuilder()
            .setDateFormat(DATE_TIME_FORMAT)
            .create();
    private static final String TASKLET_NAME_FIELD = "__tasklet_name__";
    private static final String TASKLET_INPUT_FIELD = "__tasklet_input__";
    private static final String TASKLET_OUTPUT_FIELD = "__tasklet_output__";
    private static final String TASKLET_SECRET_FIELD = "__tasklet_secret__";
    /**
     * Содержит информацию о результатах, в том числе в случае ошибки.
     */
    private static final String TASKLET_RESULT_FIELD = "tasklet_result";

    private final SandboxClient sandboxClient;
    private final String taskOwner;

    @Nullable
    private final String secretUid;

    public SandboxTaskService(SandboxClient sandboxClient, String taskOwner, @Nullable String secretUid) {
        this.sandboxClient = sandboxClient;
        this.taskOwner = taskOwner;
        this.secretUid = secretUid;
    }

    public long createTaskletTask(
            String taskType,
            String taskletImplementation,
            String description,
            JsonObject input,
            List<String> tags,
            List<String> hints,
            Map<String, Object> context,
            RuntimeSettings runtimeSettings) {

        Preconditions.checkArgument(runtimeSettings.getRequirements().getTasksResource() != 0);

        var fields = new ArrayList<>(List.of(
                new SandboxCustomField(TASKLET_NAME_FIELD, taskletImplementation),
                new SandboxCustomField(TASKLET_INPUT_FIELD, GsonPreciseDeserializer.toMap(input))
        ));

        if (runtimeSettings.getTaskletFeatures().isConsumesSecretId() && secretUid != null) {
            fields.add(
                    new SandboxCustomField(TASKLET_SECRET_FIELD, secretUid + "#" + YavSecret.CI_TOKEN)
            );
        }

        SandboxTask sandboxTask = SandboxTask.builder()
                .type(taskType)
                .owner(taskOwner.toUpperCase())
                .description(description)
                .customFields(fields)
                .context(context)
                .requirements(runtimeSettings.getRequirements())
                .notifications(runtimeSettings.getNotifications())
                .priority(runtimeSettings.getSandboxTaskPriority())
                .killTimeoutDuration(runtimeSettings.getKillTimeout())
                .tags(tags)
                .hints(hints)
                .build();

        SandboxTaskOutput sandboxTaskOutput = sandboxClient.createTask(sandboxTask);
        log.info("Task {} #{} created", taskType, sandboxTaskOutput.getId());
        return sandboxTaskOutput.getId();
    }

    public long createTask(String name,
                           String description,
                           Map<String, Object> fields,
                           List<String> tags,
                           List<String> hints,
                           Map<String, Object> context,
                           RuntimeSettings runtimeSettings,
                           SandboxTaskClass taskClass) {

        Preconditions.checkNotNull(fields);
        var customFields = fields.entrySet().stream()
                .map(entry -> createField(entry.getKey(), entry.getValue()))
                .collect(Collectors.toList());

        var builder = SandboxTask.builder()
                .owner(taskOwner.toUpperCase())
                .description(description)
                .context(context)
                .killTimeoutDuration(runtimeSettings.getKillTimeout())
                .requirements(runtimeSettings.getRequirements())
                .notifications(runtimeSettings.getNotifications())
                .priority(runtimeSettings.sandboxTaskPriority)
                .customFields(customFields)
                .tags(tags)
                .hints(hints);

        var ret = switch (taskClass) {
            case TASK -> builder.type(name);
            case TEMPLATE -> builder.templateAlias(name);
        };

        SandboxTask sandboxTask = ret.build();

        log.info("Creating sandbox task {}", sandboxTask);
        SandboxTaskOutput sandboxTaskOutput = sandboxClient.createTask(sandboxTask);
        log.info("Task {} #{} created", name, sandboxTaskOutput.getId());
        return sandboxTaskOutput.getId();
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    private static SandboxCustomField createField(String key, Object value) {
        if (value instanceof String) {
            return new SandboxCustomField(key, (String) value);
        } else if (value instanceof Number) {
            return new SandboxCustomField(key, (Number) value);
        } else if (value instanceof Map) {
            return new SandboxCustomField(key, (Map) value);
        } else if (value instanceof List) {
            return new SandboxCustomField(key, (List) value);
        } else if (value instanceof Boolean) {
            return new SandboxCustomField(key, (Boolean) value);
        }
        throw new RuntimeException(
                "unexpected field type for key " + key + ", value: " + value + ", type: " + value.getClass()
        );
    }

    public void start(long taskId) {
        BatchResult result = sandboxClient.startTask(taskId, "Task started by " + taskOwner);

        if (result.getStatus() == BatchStatus.ERROR) {
            throw new RuntimeException("Sandbox task " + taskId + " cannot start, see __last_error_trace in context: " +
                    result.getMessage());
        }
    }

    public void stop(long taskId, String comment) {
        BatchResult result = sandboxClient.stopTask(taskId, comment);

        if (result.getStatus() == BatchStatus.ERROR) {
            throw new RuntimeException("Sandbox task " + taskId + " cannot stop, see __last_error_trace in context: " +
                    result.getMessage());
        }
    }

    public SandboxTaskStatus getStatus(long taskId) {
        return sandboxClient.getTaskStatus(taskId).getData();
    }

    public TaskletResult toTaskletResult(SandboxTaskOutput sandboxTaskOutput,
                                         TaskBadgeService taskBadgeService,
                                         List<SandboxTaskBadgesConfig> badgesConfigs,
                                         String sbTaskUrl,
                                         boolean preserveTaskletOutput) {
        List<TaskBadge> taskReportBadges = taskBadgeService.toTaskBadges(sandboxTaskOutput, badgesConfigs, sbTaskUrl);

        if (!sandboxTaskOutput.getInputParameters().containsKey(TASKLET_NAME_FIELD)) {
            // Just a sandbox task (not a tasklet), return all output parameters as is
            var output = JsonParser.parseString(GSON.toJson(sandboxTaskOutput.getOutputParameters())).getAsJsonObject();
            return TaskletResult.success(output, taskReportBadges);
        }
        String resultJson = sandboxTaskOutput.getOutputParameter(TASKLET_RESULT_FIELD, String.class)
                .orElseThrow(() -> new RuntimeException("not found output parameter " + TASKLET_RESULT_FIELD));
        //TODO use parsing from JobResult after https://st.yandex-team.ru/DEVTOOLSSUPPORT-2292
        //     https://a.yandex-team.ru/arc/trunk/arcadia/tasklet/api/tasklet.proto#L43

        TaskletJobResultMessage jobResult = CiGson.instance().fromJson(
                resultJson,
                TaskletJobResultMessage.class
        );

        if (jobResult.isSuccess() || preserveTaskletOutput) {
            JsonObject taskletOutput = sandboxTaskOutput.getOutputParameter(TASKLET_OUTPUT_FIELD, Object.class)
                    .map(s -> JsonParser.parseString(GSON.toJson(s)).getAsJsonObject())
                    .orElse(null);
            return jobResult.isSuccess()
                    ? TaskletResult.success(taskletOutput, taskReportBadges)
                    : TaskletResult.notSuccess(taskletOutput, taskReportBadges);
        }
        return TaskletResult.notSuccess();
    }

    @Nullable
    public String getSecretUid() {
        return secretUid;
    }

    public List<JobResource> getTaskResources(long taskId, @Nullable Set<ResourceState> acceptResourceStates) {
        Resources resources = sandboxClient.getTaskResources(taskId, null);
        Preconditions.checkState(resources != null);

        log.info("Collected resources: total = {}, limit = {}, offset = {}, items = {}",
                resources.getTotal(), resources.getLimit(), resources.getOffset(), resources.getItems().size());

        var accept = acceptResourceStates == null || acceptResourceStates.isEmpty() ?
                Set.of(ResourceState.READY) :
                acceptResourceStates;

        var result = new ArrayList<JobResource>();
        for (var resource : resources.getItems()) {
            log.info("TaskId = {}, resource = {}", taskId, resource);

            var state = resource.getState();
            if (accept.contains(state)) {
                result.add(SandboxTaskService.taskResourceToJobResource(resource));
            } else {
                log.info("Ignore resource at state {} (accept only: {})", state, accept);
            }
        }
        return result;
    }

    private static JobResource taskResourceToJobResource(ResourceInfo resourceInfo) {
        SandboxResource sandboxResource = SandboxResource.newBuilder()
                .setId(resourceInfo.getId())
                .setType(resourceInfo.getType())
                .setTaskId(resourceInfo.getTask().getId())
                .putAllAttributes(resourceInfo.getAttributes())
                .build();
        return JobResource.regular(sandboxResource);
    }


    @Value
    private static class TaskletJobResultMessage {
        boolean success;
        String error;
    }

    @Value
    public static class TaskletResult {
        @Nullable
        JsonObject output;
        @Nullable
        List<TaskBadge> taskBadges;
        boolean success;

        public static TaskletResult notSuccess() {
            return new TaskletResult(null, null, false);
        }

        public static TaskletResult notSuccess(@Nullable JsonObject output, @Nullable List<TaskBadge> taskBadges) {
            return new TaskletResult(output, taskBadges, false);
        }

        public static TaskletResult success(@Nullable JsonObject output, @Nullable List<TaskBadge> taskBadges) {
            return new TaskletResult(output, taskBadges, true);
        }
    }

    @Value
    @Builder
    public static class RuntimeSettings {
        SandboxTaskRequirements requirements;
        List<NotificationSetting> notifications;
        @Nonnull
        Features taskletFeatures;
        @Nullable
        SandboxTaskPriority sandboxTaskPriority;
        @Nullable
        Duration killTimeout;

        public static class Builder {
            {
                taskletFeatures = Features.empty();
            }
        }
    }

    public SandboxClient getSandboxClient() {
        return sandboxClient;
    }
}
