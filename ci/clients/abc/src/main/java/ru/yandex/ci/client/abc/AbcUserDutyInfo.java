package ru.yandex.ci.client.abc;

import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class AbcUserDutyInfo {
    Long id;
    Schedule schedule;
    @JsonProperty("is_approved")
    boolean approved;
    @JsonProperty("start_datetime")
    Instant start;
    @JsonProperty("end_datetime")
    Instant end;

    public String getServiceSlug() {
        return schedule.getSlug();
    }

    @Value
    public static class Schedule {
        Long id;
        String name;
        String slug; // Schedule slug, not ABC slug
    }
}
