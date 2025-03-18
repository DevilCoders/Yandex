package ru.yandex.ci.core.config;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum ConfigStatus {
    READY,
    SECURITY_PROBLEM,
    NOT_CI,
    INVALID;

    public boolean isValidCiConfig() {
        return this == READY || this == SECURITY_PROBLEM;
    }
}
