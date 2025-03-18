package ru.yandex.ci.client.sandbox.model;

import java.time.LocalDateTime;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class Semaphore {
    long capacity;
    long value;
    LocalDateTime updateTime;
    String author;
}
