package ru.yandex.ci.core.tasklet;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum TaskletRuntime {
    SANDBOX,
    TASKLET_V2
}
