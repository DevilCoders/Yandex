package ru.yandex.ci.storage.core.db.model.test_diff;

import lombok.Builder;
import lombok.Value;

@Value
@Builder(toBuilder = true)
public class DiffPageCursor {
    Integer forwardPageId;
    Integer backwardPageId;

    public static class Builder {
        public DiffPageCursor build() {
            if (forwardPageId == null) {
                forwardPageId = 0;
            }

            if (backwardPageId == null) {
                backwardPageId = 0;
            }

            return new DiffPageCursor(forwardPageId, backwardPageId);
        }
    }
}
