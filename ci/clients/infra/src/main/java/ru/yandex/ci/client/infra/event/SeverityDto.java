package ru.yandex.ci.client.infra.event;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum SeverityDto {
    @JsonProperty("minor")
    MINOR,
    @JsonProperty("major")
    MAJOR
}
