package ru.yandex.ci.client.sandbox.api;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum ResourceState {
    BROKEN, DELETED, READY, NOT_READY
}
