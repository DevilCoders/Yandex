package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.NonNull;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = TrackerWatchConfig.Builder.class)
public class TrackerWatchConfig {

    @Nonnull
    @JsonProperty
    String queue;

    @Nullable
    @JsonProperty
    Set<String> issues;

    @Nonnull
    @JsonProperty
    String status;

    @Nullable
    @JsonProperty("close-status")
    List<String> closeStatuses;

    @Nonnull
    @JsonProperty("flow-var")
    String flowVar;

    @Nonnull
    @JsonProperty("secret")
    YavSecretSpec secret;

    public Set<String> getIssues() {
        return Objects.requireNonNullElse(issues, Set.of());
    }

    public List<String> getCloseStatuses() {
        return Objects.requireNonNullElse(closeStatuses, List.of());
    }

    @Persisted
    @Value
    @lombok.Builder
    @JsonDeserialize(builder = YavSecretSpec.Builder.class)
    public static class YavSecretSpec {
        @Nullable
        @JsonProperty
        String uuid;

        @NonNull
        @JsonProperty
        String key;
    }
}
