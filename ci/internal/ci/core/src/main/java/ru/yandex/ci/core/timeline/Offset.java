package ru.yandex.ci.core.timeline;

import lombok.Value;

@Value(staticConstructor = "of")
public class Offset {
    public static final Offset EMPTY = Offset.of(Long.MAX_VALUE, 0);

    long revisionNumber;
    int itemNumber;

    public static Offset fromRevisionNumberExclusive(long revisionNumber) {
        return of(revisionNumber, 0);
    }
}
