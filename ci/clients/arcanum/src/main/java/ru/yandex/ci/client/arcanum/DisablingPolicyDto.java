package ru.yandex.ci.client.arcanum;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum DisablingPolicyDto {
    @JsonProperty("denied")
    DENIED,
    @JsonProperty("need_reason")
    NEED_REASON,
    @JsonProperty("allowed")
    ALLOWED;
}
