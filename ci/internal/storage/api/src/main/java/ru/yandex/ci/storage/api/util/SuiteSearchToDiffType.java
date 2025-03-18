package ru.yandex.ci.storage.api.util;

import java.util.Set;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.db.model.test_diff.SuiteSearchFilters;

public class SuiteSearchToDiffType {
    private SuiteSearchToDiffType() {

    }

    public static Set<TestDiffType> convert(SuiteSearchFilters search) {
        if (!search.getSpecialCases().equals(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_NONE)) {
            return convertSpecialCase(search);
        }

        return search.getCategory().equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED) ?
                convertChanged(search) : convertAll(search);

    }

    private static Set<TestDiffType> convertChanged(SuiteSearchFilters search) {
        return switch (search.getStatus()) {
            case STATUS_ALL, UNRECOGNIZED -> Set.of();
            case STATUS_PASSED -> Set.of(TestDiffType.TDT_PASSED_NEW, TestDiffType.TDT_PASSED_FIXED);
            case STATUS_FAILED -> Set.of(TestDiffType.TDT_FAILED_NEW, TestDiffType.TDT_FAILED_BROKEN);
            case STATUS_FAILED_WITH_DEPS -> Set.of(
                    TestDiffType.TDT_FAILED_BY_DEPS_NEW, TestDiffType.TDT_FAILED_BY_DEPS_BROKEN
            );
            case STATUS_SKIPPED -> Set.of(
                    TestDiffType.TDT_SKIPPED_NEW
            );
        };
    }

    private static Set<TestDiffType> convertAll(SuiteSearchFilters search) {
        return switch (search.getStatus()) {
            case STATUS_ALL, UNRECOGNIZED -> Set.of();
            case STATUS_PASSED -> Set.of(
                    TestDiffType.TDT_PASSED_NEW, TestDiffType.TDT_PASSED_FIXED, TestDiffType.TDT_PASSED
            );
            case STATUS_FAILED -> Set.of(
                    TestDiffType.TDT_FAILED_NEW, TestDiffType.TDT_FAILED_BROKEN, TestDiffType.TDT_FAILED
            );
            case STATUS_FAILED_WITH_DEPS -> Set.of(
                    TestDiffType.TDT_FAILED_BY_DEPS_NEW, TestDiffType.TDT_FAILED_BY_DEPS_BROKEN,
                    TestDiffType.TDT_FAILED_BY_DEPS
            );
            case STATUS_SKIPPED -> Set.of(
                    TestDiffType.TDT_SKIPPED_NEW, TestDiffType.TDT_SKIPPED
            );
        };
    }

    private static Set<TestDiffType> convertSpecialCase(SuiteSearchFilters search) {
        return search.getCategory().equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED) ?
                convertSpecialCaseChanged(search) : convertSpecialCaseAll(search);
    }

    private static Set<TestDiffType> convertSpecialCaseAll(SuiteSearchFilters search) {
        return switch (search.getSpecialCases()) {
            case SPECIAL_CASE_NONE, SPECIAL_CASE_MUTED, SPECIAL_CASE_FAILED_IN_STRONG_MODE,
                    UNRECOGNIZED, SPECIAL_CASE_ADDED, SPECIAL_CASE_DELETED -> Set.of();
            case SPECIAL_CASE_EXTERNAL -> Set.of(
                    TestDiffType.TDT_EXTERNAL_FAILED, TestDiffType.TDT_EXTERNAL_BROKEN,
                    TestDiffType.TDT_EXTERNAL_NEW, TestDiffType.TDT_EXTERNAL_FIXED
            );
            case SPECIAL_CASE_INTERNAL -> Set.of(
                    TestDiffType.TDT_INTERNAL_FAILED, TestDiffType.TDT_INTERNAL_BROKEN,
                    TestDiffType.TDT_INTERNAL_NEW, TestDiffType.TDT_INTERNAL_FIXED
            );
            case SPECIAL_CASE_TIMEOUT -> Set.of(
                    TestDiffType.TDT_TIMEOUT_FAILED, TestDiffType.TDT_TIMEOUT_BROKEN,
                    TestDiffType.TDT_TIMEOUT_NEW, TestDiffType.TDT_TIMEOUT_FIXED
            );
            case SPECIAL_CASE_FLAKY -> Set.of(
                    TestDiffType.TDT_FLAKY_FAILED, TestDiffType.TDT_FLAKY_BROKEN,
                    TestDiffType.TDT_FLAKY_NEW, TestDiffType.TDT_FLAKY_FIXED
            );
            case SPECIAL_CASE_BROKEN_BY_DEPS -> Set.of(
                    TestDiffType.TDT_FAILED_BY_DEPS, TestDiffType.TDT_FAILED_BY_DEPS_BROKEN,
                    TestDiffType.TDT_FAILED_BY_DEPS_NEW
            );
        };
    }

    private static Set<TestDiffType> convertSpecialCaseChanged(SuiteSearchFilters search) {
        return switch (search.getSpecialCases()) {
            case SPECIAL_CASE_NONE, SPECIAL_CASE_MUTED, SPECIAL_CASE_FAILED_IN_STRONG_MODE,
                    UNRECOGNIZED -> Set.of();
            case SPECIAL_CASE_EXTERNAL -> Set.of(
                    TestDiffType.TDT_EXTERNAL_BROKEN, TestDiffType.TDT_EXTERNAL_NEW, TestDiffType.TDT_EXTERNAL_FIXED
            );
            case SPECIAL_CASE_INTERNAL -> Set.of(
                    TestDiffType.TDT_INTERNAL_BROKEN, TestDiffType.TDT_INTERNAL_NEW, TestDiffType.TDT_INTERNAL_FIXED
            );
            case SPECIAL_CASE_TIMEOUT -> Set.of(
                    TestDiffType.TDT_TIMEOUT_BROKEN, TestDiffType.TDT_TIMEOUT_NEW, TestDiffType.TDT_TIMEOUT_FIXED
            );
            case SPECIAL_CASE_FLAKY -> Set.of(
                    TestDiffType.TDT_FLAKY_BROKEN, TestDiffType.TDT_FLAKY_NEW, TestDiffType.TDT_FLAKY_FIXED
            );
            case SPECIAL_CASE_ADDED -> Set.of(
                    TestDiffType.TDT_PASSED_NEW,
                    TestDiffType.TDT_FAILED_NEW,
                    TestDiffType.TDT_EXTERNAL_NEW,
                    TestDiffType.TDT_INTERNAL_NEW,
                    TestDiffType.TDT_TIMEOUT_NEW,
                    TestDiffType.TDT_FLAKY_NEW,
                    TestDiffType.TDT_FAILED_BY_DEPS_NEW
            );
            case SPECIAL_CASE_DELETED -> Set.of(TestDiffType.TDT_DELETED);
            case SPECIAL_CASE_BROKEN_BY_DEPS -> Set.of(
                    TestDiffType.TDT_FAILED_BY_DEPS_BROKEN, TestDiffType.TDT_FAILED_BY_DEPS_NEW
            );
        };
    }
}
