package ru.yandex.ci.storage.core.db.model.check_task;

import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class LargeTestDelegatedConfig {
    String path;
    OrderedArcRevision revision;
}
