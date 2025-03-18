package ru.yandex.ci.core.config.a.model;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.gson.JsonElement;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.config.ConfigIdEntry;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.util.gson.GsonElementSerializer;
import ru.yandex.ci.util.gson.ToGsonElementDeserializer;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = JobConfig.Builder.class)
public class JobConfig implements ConfigIdEntry<JobConfig>, HasParseInfo {
    @JsonIgnore
    @Getter(onMethod_ = @Override)
    @With(onMethod_ = @Override)
    String id;

    @JsonProperty
    String title;

    @JsonProperty
    String description;

    @JsonProperty
    String task;

    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty
    List<String> needs;

    @JsonProperty
    String stage;

    @JsonProperty
    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonSerialize(converter = GsonElementSerializer.class)
    JsonElement input;

    @JsonProperty("context-input")
    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonSerialize(converter = GsonElementSerializer.class)
    JsonElement contextInput;

    @JsonProperty
    JobMultiplyConfig multiply;

    @JsonProperty
    ManualConfig manual;

    @JsonProperty
    RequirementsConfig requirements;

    @JsonProperty
    String version;

    @Nullable
    @JsonProperty
    JobAttemptsConfig attempts;

    @JsonProperty("needs-type")
    NeedsType needsType;

    @JsonProperty("if")
    String runIf;

    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    @JsonProperty("kill-timeout")
    Duration killTimeout;

    @Nullable
    @JsonProperty("runtime")
    RuntimeConfig jobRuntimeConfig;

    // Internal property, configured manually only on auto-rollback flows
    @Nullable
    @JsonProperty("skipped-by")
    JobSkippedByMode skippedByMode;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @JsonIgnore
    @NonFinal
    transient ParseInfo parseInfo;

    public String getTitle() {
        return Objects.requireNonNullElse(title, id);
    }

    // Only manual configuration
    public JobConfig withSkippedBy(JobSkippedByMode skippedByMode) {
        var skippedTitle = switch (skippedByMode) {
            case STAGE -> "Stage";
            case REGISTRY -> "Registry";
        };
        return toBuilder()
                .skippedByMode(skippedByMode)
                .title("Skip by " + skippedTitle + ": " + getTitle())
                .runIf("${`false`}")
                .manual(ManualConfig.of(false))
                .build();
    }

    @JsonIgnore
    public TaskId getTaskId() {
        return TaskId.of(task);
    }

    public static class Builder {
        {
            needs = new ArrayList<>();
            needsType = NeedsType.ALL;
            manual = ManualConfig.of(false);
        }
    }
}
