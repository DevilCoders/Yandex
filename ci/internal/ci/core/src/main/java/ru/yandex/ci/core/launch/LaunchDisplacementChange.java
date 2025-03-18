package ru.yandex.ci.core.launch;

import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@BenderBindAllFields
@Persisted
@Value(staticConstructor = "of")
public class LaunchDisplacementChange {

    @Nonnull
    Common.DisplacementState state;

    @Nonnull
    String changedBy;

    @Nonnull
    Instant changedAt;

}
