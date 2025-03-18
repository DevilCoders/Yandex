package ru.yandex.ci.storage.core.db.model.test_revision;

import lombok.Builder;
import lombok.Value;

@Value
@Builder
public class HistoryPageCursor {
    HistoryPaging next;
    HistoryPaging previous;
}
