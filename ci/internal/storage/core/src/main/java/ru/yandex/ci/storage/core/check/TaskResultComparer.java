package ru.yandex.ci.storage.core.check;

import java.util.HashMap;
import java.util.Map;

import com.google.common.base.Preconditions;

import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.Common.TestStatus;

@SuppressWarnings("DuplicatedCode")
public class TaskResultComparer {
    private static final Map<TestStatus, Map<TestStatus, TestDiffType>> MAP = new HashMap<>();

    private TaskResultComparer() {

    }

    static {
        MAP.put(TestStatus.TS_UNKNOWN, fillOnLeftIsUnknown());
        MAP.put(TestStatus.TS_NONE, fillOnLeftIsNone());
        MAP.put(TestStatus.TS_DISCOVERED, fillOnLeftIsDiscovered());
        MAP.put(TestStatus.TS_FAILED, fillOnLeftIsFailed());
        MAP.put(TestStatus.TS_FLAKY, fillOnLeftIsFlaky());
        MAP.put(TestStatus.TS_INTERNAL, fillOnLeftIsInternal());
        MAP.put(TestStatus.TS_OK, fillOnLeftIsOk());
        MAP.put(TestStatus.TS_SKIPPED, fillOnLeftIsSkipped());
        MAP.put(TestStatus.TS_TIMEOUT, fillOnLeftIsTimeout());
        MAP.put(TestStatus.TS_BROKEN_DEPS, fillOnLeftIsBrokenDeps());
        MAP.put(TestStatus.TS_XFAILED, fillOnLeftIsOk());
        MAP.put(TestStatus.TS_XPASSED, fillOnLeftIsFailed());
        MAP.put(TestStatus.TS_SUITE_PROBLEMS, fillOnLeftIsSuiteProblems());
        MAP.put(TestStatus.TS_NOT_LAUNCHED, fillOnLeftIsSuiteProblems());
    }

    // [UNKNOWN] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsUnknown() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [NONE] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsNone() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED_NEW);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED_NEW);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_NEW);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_NEW);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED_NEW);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED_NEW);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_NEW);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS_NEW);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED_NEW);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED_NEW);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [DISCOVERED] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsDiscovered() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_BROKEN);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_BROKEN);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_BROKEN);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [FAILED | XPASSED] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsFailed() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED_FIXED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED_FIXED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [FLAKY] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsFlaky() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_FLAKY_FIXED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED_FIXED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [INTERNAL] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsInternal() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_INTERNAL_FIXED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_INTERNAL_FIXED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [OK | XFAILED] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsOk() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_BROKEN);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_BROKEN);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_BROKEN);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS_BROKEN);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [SKIPPED] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsSkipped() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_BROKEN);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_BROKEN);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_BROKEN);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS_BROKEN);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED_BROKEN);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [TIMEOUT] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsTimeout() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_TIMEOUT_FIXED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_TIMEOUT_FIXED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [BROKEN_DEPS] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsBrokenDeps() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_UNKNOWN);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_DELETED);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_BROKEN);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_BROKEN);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED_BY_DEPS_FIXED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED_BY_DEPS_FIXED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED_NEW);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    // [SUITE_PROBLEMS] ->
    private static Map<TestStatus, TestDiffType> fillOnLeftIsSuiteProblems() {
        var mapping = new HashMap<TestStatus, TestDiffType>();
        mapping.put(TestStatus.TS_UNKNOWN, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NONE, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_DISCOVERED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_FAILED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_FLAKY, TestDiffType.TDT_FLAKY_FAILED);
        mapping.put(TestStatus.TS_INTERNAL, TestDiffType.TDT_INTERNAL_FAILED);
        mapping.put(TestStatus.TS_OK, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_SKIPPED, TestDiffType.TDT_SKIPPED);
        mapping.put(TestStatus.TS_TIMEOUT, TestDiffType.TDT_TIMEOUT_FAILED);
        mapping.put(TestStatus.TS_BROKEN_DEPS, TestDiffType.TDT_FAILED_BY_DEPS);
        mapping.put(TestStatus.TS_XFAILED, TestDiffType.TDT_PASSED);
        mapping.put(TestStatus.TS_XPASSED, TestDiffType.TDT_FAILED);
        mapping.put(TestStatus.TS_SUITE_PROBLEMS, TestDiffType.TDT_SUITE_PROBLEMS);
        mapping.put(TestStatus.TS_NOT_LAUNCHED, TestDiffType.TDT_SKIPPED);

        return mapping;
    }

    public static TestDiffType compare(TestStatus left, TestStatus right, boolean isExternal) {
        Preconditions.checkState(!left.equals(TestStatus.UNRECOGNIZED), "left not recognized");
        Preconditions.checkState(!right.equals(TestStatus.UNRECOGNIZED), "right not recognized");

        if (left == TestStatus.TS_MULTIPLE_PROBLEMS) {
            left = TestStatus.TS_FAILED;
        }

        if (right == TestStatus.TS_MULTIPLE_PROBLEMS) {
            right = TestStatus.TS_FAILED;
        }

        var diff = MAP.get(left).get(right);
        if (isExternal) {
            return modifyExternal(diff);
        }

        return diff;
    }

    private static TestDiffType modifyExternal(TestDiffType diff) {
        return switch (diff) {
            case TDT_PASSED_NEW, TDT_PASSED_FIXED -> TestDiffType.TDT_EXTERNAL_FIXED;
            case TDT_FAILED -> TestDiffType.TDT_EXTERNAL_FAILED;
            case TDT_FAILED_BROKEN, TDT_FAILED_NEW -> TestDiffType.TDT_EXTERNAL_BROKEN;
            default -> diff;
        };
    }
}
