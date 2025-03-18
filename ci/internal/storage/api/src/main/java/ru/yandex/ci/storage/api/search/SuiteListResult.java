package ru.yandex.ci.storage.api.search;

import java.util.List;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;

@Value
public class SuiteListResult {
    List<TestDiffEntity> diffs;

    boolean hasMore;
}
