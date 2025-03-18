package ru.yandex.ci.core.arc;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class Ticket {
    @Nonnull
    String queue;
    @Nonnull
    String key;
}
