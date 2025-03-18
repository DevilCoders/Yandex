package ru.yandex.ci.client.sandbox.api;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum SandboxTaskGroupStatus {
    @JsonEnumDefaultValue
    UNKNOWN,

    FINISH,
    EXECUTE,
    QUEUE,
    DRAFT,
    WAIT,
    BREAK
}
