package ru.yandex.ci.storage.core.db.model.yt;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum YtExportEntityType {
    TEST_RESULT,
    TEST_DIFF
}
