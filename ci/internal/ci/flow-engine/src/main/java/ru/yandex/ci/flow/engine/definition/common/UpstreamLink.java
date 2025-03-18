package ru.yandex.ci.flow.engine.definition.common;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class UpstreamLink<T> {
    @Nonnull
    T entity;

    @Nonnull
    UpstreamType type;

    @Nonnull
    UpstreamStyle style;
}

