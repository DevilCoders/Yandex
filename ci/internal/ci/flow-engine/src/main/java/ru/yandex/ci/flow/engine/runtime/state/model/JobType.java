package ru.yandex.ci.flow.engine.runtime.state.model;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum JobType {
    DEFAULT, CLEANUP
}
