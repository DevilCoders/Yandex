package ru.yandex.ci.core.config.a;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum ConfigChangeType {
    NONE,
    ADD,
    DELETE,
    MODIFY;

    public boolean isExisting() {
        return this != DELETE;
    }
}
