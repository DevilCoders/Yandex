package ru.yandex.ci.client.sandbox.api;

import java.time.Instant;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class TaskExecution {
    Instant started;
    Instant finished;
}
