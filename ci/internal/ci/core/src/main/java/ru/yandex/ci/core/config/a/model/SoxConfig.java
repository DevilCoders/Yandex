package ru.yandex.ci.core.config.a.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = SoxConfig.Builder.class)
public class SoxConfig {
    @JsonProperty("approval-scope")
    String approvalScope;
}
