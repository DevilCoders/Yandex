package ru.yandex.ci.client.sandbox.api;

import java.time.LocalDateTime;

import lombok.Value;

@Value
public class SemaphoreAuditResponse {
    String author;
    LocalDateTime time;
}
