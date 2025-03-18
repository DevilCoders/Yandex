package ru.yandex.ci.client.juggler.model;

import java.time.Instant;
import java.util.List;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class RawEvent {
    String host;
    String service;
    String instance;
    Status status;
    String description;
    List<String> tags;
    Instant openTime;
    Integer heartbeat;
}
