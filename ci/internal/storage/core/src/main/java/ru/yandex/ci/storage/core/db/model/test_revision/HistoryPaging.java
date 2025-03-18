package ru.yandex.ci.storage.core.db.model.test_revision;

import lombok.Value;

@Value
public class HistoryPaging {
    public static final HistoryPaging EMPTY = new HistoryPaging(0, 0);

    // Revisions displayed in reversed order, so `fromRevision` is always greater than `toRevision`.
    long fromRevision;
    long toRevision;

    public static HistoryPaging from(long revision) {
        return new HistoryPaging(revision, 0);
    }

    public static HistoryPaging to(long revision) {
        return new HistoryPaging(0, revision);
    }
}
