package ru.yandex.ci.core.config.registry;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = TaskletConfig.Builder.class)
public class TaskletConfig {
    @JsonProperty
    String implementation;

    @JsonProperty("single-input")
    @JsonAlias("singleInput")
    boolean singleInput;

    @JsonProperty("single-output")
    @JsonAlias("singleOutput")
    boolean singleOutput;

    @JsonIgnoreProperties(ignoreUnknown = true) // runtime: sandbox
    public static class Builder {

    }

}
