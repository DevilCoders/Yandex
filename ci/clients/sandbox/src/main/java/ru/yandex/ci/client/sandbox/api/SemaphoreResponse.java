package ru.yandex.ci.client.sandbox.api;

import java.time.LocalDateTime;
import java.util.Map;

import lombok.Value;

@Value
public class SemaphoreResponse {
    long capacity;
    long value;
    Map<String, LocalDateTime> time;
}
