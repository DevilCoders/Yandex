package ru.yandex.ci.storage.core.db.model.check;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class DistbuildPriority {
    long fixedPriority;
    long priorityRevision;
}
