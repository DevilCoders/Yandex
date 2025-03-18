package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.Singular;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.config.ConfigIdEntry;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.util.gson.GsonElementSerializer;
import ru.yandex.ci.util.gson.ToGsonElementDeserializer;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@Value
@Builder
@JsonDeserialize(builder = ActionConfig.Builder.class)
public class ActionConfig implements ConfigIdEntry<ActionConfig>, HasParseInfo, HasFlowRef {

    @With(onMethod_ = @Override)
    @Getter(onMethod_ = @Override)
    @JsonIgnore
    String id; // Possibly null during initialization but never null when fully loaded

    @Nullable
    @JsonProperty
    String title;

    @Nullable
    @JsonProperty
    String description;

    @Nonnull
    @Getter(onMethod_ = @Override)
    @JsonProperty
    String flow;

    @Nonnull
    @Singular
    @JsonProperty
    List<TriggerConfig> triggers;

    @Nullable
    @JsonProperty("flow-vars")
    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonSerialize(converter = GsonElementSerializer.class)
    JsonObject flowVars;

    @Nullable
    @JsonProperty("flow-vars-ui")
    FlowVarsUi flowVarsUi;

    @Nullable
    @JsonProperty("cleanup")
    CleanupConfig cleanupConfig;

    @Nullable
    @JsonProperty
    RequirementsConfig requirements;

    @Nullable
    @JsonProperty("runtime")
    RuntimeConfig runtimeConfig;

    @Nonnull
    @Singular
    @JsonProperty
    List<String> tags;

    @JsonProperty
    Permissions permissions;

    @JsonProperty("max-active")
    Integer maxActiveCount;

    @JsonProperty("max-start-per-minute")
    Integer maxStartPerMinute;

    @Nullable
    @JsonProperty("binary-search")
    BinarySearchConfig binarySearchConfig;

    @JsonProperty("tracker-watcher")
    TrackerWatchConfig trackerWatchConfig;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @NonFinal
    @JsonIgnore
    transient ParseInfo parseInfo;

    @JsonIgnore
    boolean virtual;

    public boolean hastMaxActiveCount() {
        return Objects.requireNonNullElse(maxActiveCount, 0) > 0;
    }

    @Nonnull
    public List<String> getTags() {
        return Objects.requireNonNullElse(tags, List.of());
    }

    @JsonIgnoreProperties(ignoreUnknown = true) // cleanupTrigger
    public static class Builder {

    }
}
