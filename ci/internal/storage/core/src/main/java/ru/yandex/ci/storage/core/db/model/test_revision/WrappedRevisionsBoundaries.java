package ru.yandex.ci.storage.core.db.model.test_revision;

import lombok.Value;

@Value
public class WrappedRevisionsBoundaries {
    long from;
    long to;

    public String generateName() {
        return "[%d-%d]".formatted(from, to);
    }
}
