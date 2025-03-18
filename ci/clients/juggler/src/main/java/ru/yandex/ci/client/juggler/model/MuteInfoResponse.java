package ru.yandex.ci.client.juggler.model;

import java.time.Instant;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class MuteInfoResponse {
    @JsonProperty("mute_id")
    String id;

    String user;

    Instant startTime;

    Instant endTime;
}
