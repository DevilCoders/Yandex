package ru.yandex.ci.storage.core.db.model.test_revision;

import lombok.Value;

import ru.yandex.ci.storage.core.Common;

@Value
public class HistoryFilters {
    Common.TestStatus status;
    long fromRevision;
    long toRevision;
}
