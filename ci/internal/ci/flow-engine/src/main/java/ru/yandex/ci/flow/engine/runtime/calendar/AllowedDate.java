package ru.yandex.ci.flow.engine.runtime.calendar;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

@Value
public class AllowedDate {

    private static final AllowedDate ANY_DATE = new AllowedDate(Type.ANY_ALLOWED, null);
    private static final AllowedDate DATE_NOT_FOUND = new AllowedDate(Type.DATE_NOT_FOUND, null);

    Type type;
    @Nullable
    Instant instant;

    public static AllowedDate anyDateAllowed() {
        return ANY_DATE;
    }

    public static AllowedDate dateNotFound() {
        return DATE_NOT_FOUND;
    }

    public static AllowedDate of(@Nonnull Instant instant) {
        return new AllowedDate(Type.DATE_FOUND, instant);
    }

    public enum Type {
        ANY_ALLOWED,
        DATE_NOT_FOUND,
        DATE_FOUND
    }

}
