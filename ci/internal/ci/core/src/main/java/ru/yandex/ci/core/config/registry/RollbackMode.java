package ru.yandex.ci.core.config.registry;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum RollbackMode {
    EXECUTE,
    SKIP,
    DENY
}
