package ru.yandex.ci.core.internal;

import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class InternalExecutorContext {
    @Nonnull
    UUID executor;
}
