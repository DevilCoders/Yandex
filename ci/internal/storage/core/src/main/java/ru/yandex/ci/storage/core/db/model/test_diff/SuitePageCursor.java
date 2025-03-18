package ru.yandex.ci.storage.core.db.model.test_diff;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class SuitePageCursor {
    @Nullable
    Page forward;

    @Nullable
    Page backward;

    @Value
    public static class Page {
        Long suiteId;
        String path;
    }
}
