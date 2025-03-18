package ru.yandex.ci.client.infra.event;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum TypeDto {
    @JsonProperty("issue")
    ISSUE,
    @JsonProperty("maintenance")
    MAINTENANCE
}
