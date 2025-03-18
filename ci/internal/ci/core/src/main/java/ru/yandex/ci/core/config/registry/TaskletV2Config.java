package ru.yandex.ci.core.config.registry;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = TaskletV2Config.Builder.class)
public class TaskletV2Config {
    @JsonProperty
    String namespace;

    @JsonProperty
    String tasklet;

    @JsonProperty("single-input")
    boolean singleInput;

    @JsonProperty("single-output")
    boolean singleOutput;

}
