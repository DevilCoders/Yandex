package ru.yandex.ci.core.config.registry;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Builder
@JsonDeserialize(builder = SandboxTaskBadgesConfig.Builder.class)
@Persisted
public class SandboxTaskBadgesConfig {
    @JsonProperty
    @Nonnull
    String id;

    @JsonProperty
    @Nullable
    String module;

    public static SandboxTaskBadgesConfig of(String id) {
        return new SandboxTaskBadgesConfig(id, null);
    }

    public static SandboxTaskBadgesConfig of(String id, String module) {
        return new SandboxTaskBadgesConfig(id, module);
    }
}
