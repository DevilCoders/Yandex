package ru.yandex.ci.client.calendar.api;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum DayType {
    @JsonProperty("holiday")
    HOLIDAY,
    @JsonProperty("weekend")
    WEEKEND,
    @JsonProperty("weekday")
    WEEKDAY
}
