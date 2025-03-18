package ru.yandex.ci.client.juggler.model;

import java.time.Instant;
import java.util.List;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Value;

@Value
@JsonInclude(JsonInclude.Include.NON_EMPTY)
public class MuteCreateRequest {
    String description;

    Instant startTime;

    Instant endTime;

    List<FilterRequest> filters;
}
