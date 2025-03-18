package ru.yandex.ci.client.sandbox.api;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class SemaphoreUpdate {
    Long capacity;
    String event;
}
