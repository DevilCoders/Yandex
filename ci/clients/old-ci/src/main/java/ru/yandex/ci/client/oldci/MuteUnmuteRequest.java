package ru.yandex.ci.client.oldci;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class MuteUnmuteRequest {
    @JsonProperty("notifications_enabled")
    boolean notificationsEnabled;
    String comment;
}
