package ru.yandex.ci.flow.engine.runtime.di.model;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class StoredResourceModification {
    @Nonnull
    Instant timestamp;

    @Nonnull
    String userId;

    @Nonnull
    String diff;
}
