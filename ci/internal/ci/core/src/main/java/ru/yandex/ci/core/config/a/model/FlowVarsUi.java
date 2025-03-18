package ru.yandex.ci.core.config.a.model;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = FlowVarsUi.Builder.class)
public class FlowVarsUi {
    @Nonnull
    @JsonProperty
    JsonNode schema;
}
