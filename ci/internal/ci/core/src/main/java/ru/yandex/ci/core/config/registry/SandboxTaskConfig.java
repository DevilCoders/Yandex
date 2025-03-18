package ru.yandex.ci.core.config.registry;

import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.ResourceState;

@Value
@Builder
@JsonDeserialize(builder = SandboxTaskConfig.Builder.class)
public class SandboxTaskConfig {

    // Expect one of 'name' or 'template'
    @Nullable
    @JsonProperty
    String name;

    @Nullable
    @JsonProperty
    String template;

    @JsonProperty("required-parameters")
    @Singular
    List<String> requiredParameters;

    @JsonProperty("badges-configs")
    @Singular
    List<SandboxTaskBadgesConfig> badgesConfigs;

    @JsonProperty("accept-resource-states")
    @Singular
    Set<ResourceState> acceptResourceStates;


    public static SandboxTaskConfig ofName(String name) {
        return SandboxTaskConfig.builder()
                .name(name)
                .build();
    }

    public static SandboxTaskConfig ofTemplate(String template) {
        return SandboxTaskConfig.builder()
                .template(template)
                .build();
    }
}
