package ru.yandex.ci.client.sandbox;

import java.time.OffsetDateTime;

import lombok.Value;

@Value(staticConstructor = "of")
public class TimeRange {
    OffsetDateTime from;
    OffsetDateTime to;

    @Override
    public String toString() {
        return SandboxDateTimeFormatter.format(from, to);
    }
}
