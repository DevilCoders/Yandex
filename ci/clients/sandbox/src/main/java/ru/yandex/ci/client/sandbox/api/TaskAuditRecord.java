package ru.yandex.ci.client.sandbox.api;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class TaskAuditRecord {
    @Nullable
    SandboxTaskStatus status;
    Instant time;
    @Nullable
    String message;
    @Nullable
    String author;
}
