package ru.yandex.ci.storage.core.db.constant;

import java.util.Set;

import ru.yandex.ci.storage.core.Common.TestDiffType;

public class TestDiffTypeUtils {
    public static final Set<TestDiffType> NOTIFIED = Set.of(
            TestDiffType.TDT_PASSED_FIXED,
            TestDiffType.TDT_PASSED_BY_DEPS_FIXED,
            TestDiffType.TDT_FAILED_BROKEN,
            TestDiffType.TDT_INTERNAL_FIXED,
            TestDiffType.TDT_INTERNAL_BROKEN
    );

    private static final Set<TestDiffType> PASSED = Set.of(
            TestDiffType.TDT_PASSED,
            TestDiffType.TDT_PASSED_FIXED,
            TestDiffType.TDT_PASSED_NEW,
            TestDiffType.TDT_PASSED_BY_DEPS_FIXED
    );

    private static final Set<TestDiffType> ADDED_FAILURE = Set.of(
            TestDiffType.TDT_FAILED_BROKEN,
            TestDiffType.TDT_FAILED_NEW,
            TestDiffType.TDT_TIMEOUT_BROKEN,
            TestDiffType.TDT_TIMEOUT_NEW,
            TestDiffType.TDT_EXTERNAL_BROKEN,
            TestDiffType.TDT_EXTERNAL_NEW,
            TestDiffType.TDT_INTERNAL_FAILED,
            TestDiffType.TDT_INTERNAL_NEW
    );

    private TestDiffTypeUtils() {

    }

    public static boolean isPassed(TestDiffType value) {
        return PASSED.contains(value);
    }

    public static boolean isAddedFailure(TestDiffType value) {
        return ADDED_FAILURE.contains(value);
    }
}
