package ru.yandex.ci.flow.engine.definition.common;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class SchedulerConstraintModifications {

    @Nonnull
    String modifiedBy;

    @Nonnull
    Instant timestamp;
}
