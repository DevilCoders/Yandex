package ru.yandex.ci.tms.task.potato.client;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class Status {
    boolean healthy;
    @Nullable
    Long lastTs;
}
