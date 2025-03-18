package ru.yandex.ci.storage.api.search;

import java.util.List;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;

@Value
public class DiffListResult {
    TestDiffEntity diff;
    List<DiffWithRuns> children;

    @Value
    public static class DiffWithRuns {
        TestDiffEntity diff;
        List<TestResultEntity> runs;
    }
}
