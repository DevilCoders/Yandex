package ru.yandex.ci.core.config.a.model.auto;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum DayType {
    @JsonProperty("workdays")
    WORKDAYS,
    @JsonProperty("holidays")
    HOLIDAYS,
    @JsonProperty("not-pre-holidays")
    NOT_PRE_HOLIDAYS,
}
