package ru.yandex.ci.core.config.a.model;

import java.time.Duration;
import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class CleanupConfig {

    public static final Set<LaunchState.Status> DEFAULT_OPTIONS = Set.of(
            LaunchState.Status.SUCCESS,
            LaunchState.Status.FAILURE);

    @Nullable
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    @JsonProperty("delay")
    Duration withDelay;

    @Nullable
    @JsonProperty("on-status")
    Set<LaunchState.Status> onStatus;

    @Nullable
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty("conditions")
    List<CleanupConditionConfig> conditions;

    public Set<LaunchState.Status> getOnStatus() {
        return (onStatus == null || onStatus.isEmpty()) ? DEFAULT_OPTIONS : onStatus;
    }

    public List<CleanupConditionConfig> getConditions() {
        return Objects.requireNonNullElse(conditions, List.of());
    }
}
