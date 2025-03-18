package ru.yandex.ci.storage.core.db.model.check_task;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ExpectedTask {
    String jobName;
    boolean right;
}
