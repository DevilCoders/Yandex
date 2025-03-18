package ru.yandex.ci.core.config.a.model.auto;

import java.time.LocalTime;
import java.time.ZoneId;

import javax.annotation.Nonnull;

import lombok.Value;

@Value(staticConstructor = "of")
public class TimeRange {
    @Nonnull
    LocalTime start;

    @Nonnull
    LocalTime end;

    @Nonnull
    ZoneId zoneId;
}
