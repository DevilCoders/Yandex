package ru.yandex.ci.storage.core.db.model.sequence;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum DbSequence {
    METRIC_ID,
    TEST_ID, // te
    RUN_ID // te
}
