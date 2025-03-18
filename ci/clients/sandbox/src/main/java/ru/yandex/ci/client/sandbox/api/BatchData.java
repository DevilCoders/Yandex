package ru.yandex.ci.client.sandbox.api;

import java.util.Set;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class BatchData {
    String comment;

    Set<Long> id;
}
