package ru.yandex.ci.client.sandbox.api;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

@Builder
@Value
@AllArgsConstructor
public class BatchResult {
    public static final BatchResult EMPTY = builder().build();

    long id;

    @Nullable
    String message;

    @Nullable
    BatchStatus status;
}
