package ru.yandex.ci.core.sandbox;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class SandboxTaskRef {
    String type;
}
