package ru.yandex.ci.core.config.a.model;

import java.time.Duration;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.time.DurationDeserializer;
import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = RuntimeSandboxConfig.Builder.class)
public class RuntimeSandboxConfig implements Overridable<RuntimeSandboxConfig> {
    @JsonProperty
    String owner; // Must be non-null in the end

    @Singular
    @JsonProperty
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<SandboxNotificationConfig> notifications;

    @Singular
    @JsonProperty
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> tags;

    @Singular
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> hints;

    @Nullable
    @JsonDeserialize(converter = DurationDeserializer.class)
    @JsonSerialize(converter = DurationSerializer.class)
    @JsonProperty("kill-timeout")
    Duration killTimeout;

    @JsonProperty
    TaskPriority priority;

    @JsonProperty("keep-polling-stopped")
    Boolean keepPollingStopped;

    @Override
    public RuntimeSandboxConfig override(RuntimeSandboxConfig override) {
        var copy = RuntimeSandboxConfig.builder();
        Overrider<RuntimeSandboxConfig> overrider = new Overrider<>(this, override);
        overrider.field(copy::owner, RuntimeSandboxConfig::getOwner);
        overrider.field(copy::notifications, RuntimeSandboxConfig::getNotifications);
        overrider.field(copy::killTimeout, RuntimeSandboxConfig::getKillTimeout);
        overrider.extendDistinctList(copy::tags, RuntimeSandboxConfig::getTags);
        overrider.extendDistinctList(copy::hints, RuntimeSandboxConfig::getHints);
        overrider.field(copy::priority, RuntimeSandboxConfig::getPriority);
        overrider.field(copy::keepPollingStopped, RuntimeSandboxConfig::getKeepPollingStopped);
        return copy.build();
    }

    public static RuntimeSandboxConfig ofOwner(String owner) {
        return builder().owner(owner).build();
    }
}
