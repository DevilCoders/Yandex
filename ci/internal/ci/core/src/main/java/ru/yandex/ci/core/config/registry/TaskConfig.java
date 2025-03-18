package ru.yandex.ci.core.config.registry;

import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;
import org.apache.logging.log4j.util.Strings;

import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.registry.sandbox.InternalTaskConfig;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.util.gson.ToGsonElementDeserializer;

@Value
@Builder
@JsonDeserialize(builder = TaskConfig.Builder.class)
public class TaskConfig {
    @JsonProperty
    String title;

    @JsonProperty
    @Nullable
    String description;

    @JsonProperty
    @Nullable
    String maintainers;

    @JsonProperty
    @Nullable
    String sources; // For Tasks/Tasklets only

    @Nullable
    @JsonProperty
    String deprecated;

    @Nullable
    @JsonProperty
    TaskletConfig tasklet;

    @Nullable
    @JsonProperty("tasklet-v2")
    TaskletV2Config taskletV2;

    @Nullable
    @JsonProperty("sandbox-task")
    SandboxTaskConfig sandboxTask;

    @Nullable
    @JsonProperty("internal-task")
    InternalTaskConfig internalTask;

    @Singular
    @JsonProperty
    @JsonDeserialize(keyUsing = LaunchEnvironmentKeyDeserializer.class)
    Map<TaskVersion, String> versions;

    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonAlias("parameters")
    JsonObject resources;

    @JsonProperty
    RequirementsConfig requirements;

    @JsonProperty("runtime")
    RuntimeConfig runtimeConfig;

    @Singular
    @JsonProperty("validate")
    List<ValidationConfig> validations;

    @Nullable
    @JsonProperty("attempts")
    JobAttemptsConfig attempts;

    @Nullable
    @JsonProperty("auto-rollback-mode")
    RollbackMode autoRollbackMode;

    public RollbackMode getAutoRollbackMode() {
        return Objects.requireNonNullElse(autoRollbackMode, RollbackMode.DENY);
    }

    @JsonIgnore
    public TaskType getTaskType() {
        if (tasklet != null) {
            return TaskType.TASKLET;
        }
        if (taskletV2 != null) {
            return TaskType.TASKLET_V2;
        }
        if (internalTask != null) {
            return TaskType.INTERNAL;
        }
        Preconditions.checkState(sandboxTask != null);
        if (Strings.isNotEmpty(sandboxTask.getTemplate())) {
            return TaskType.SANDBOX_TEMPLATE;
        }
        Preconditions.checkState(Strings.isNotEmpty(sandboxTask.getName()));
        return TaskType.SANDBOX_TASK;
    }

    public enum TaskType {
        TASKLET, TASKLET_V2, INTERNAL, SANDBOX_TASK, SANDBOX_TEMPLATE
    }
}
