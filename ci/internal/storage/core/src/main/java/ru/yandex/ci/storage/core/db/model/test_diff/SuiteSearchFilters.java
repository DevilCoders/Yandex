package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.Locale;
import java.util.Set;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
public class SuiteSearchFilters {
    StorageFrontApi.StatusFilter status;

    Set<Common.ResultType> resultTypes;

    StorageFrontApi.CategoryFilter category;
    StorageFrontApi.SpecialCasesFilter specialCases;

    String toolchain;
    String path;

    String testName;
    String subtestName;

    SuitePageCursor pageCursor;

    public static class Builder {
        public SuiteSearchFilters build() {
            if (status == null) {
                status = StorageFrontApi.StatusFilter.STATUS_ALL;
            }

            if (category == null) {
                category = StorageFrontApi.CategoryFilter.CATEGORY_ALL;
            }

            if (resultTypes == null) {
                resultTypes = Set.of();
            }

            if (specialCases == null) {
                specialCases = StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_NONE;
            }

            if (toolchain == null || toolchain.isEmpty()) {
                toolchain = TestEntity.ALL_TOOLCHAINS;
            }

            if (path == null) {
                path = "";
            } else {
                path = path.replaceAll("[%]", "");
            }

            if (pageCursor == null) {
                pageCursor = new SuitePageCursor(null, null);
            }

            if (testName == null) {
                testName = "";
            } else {
                testName = testName.replaceAll("%", "");
            }

            if (subtestName == null) {
                subtestName = "";
            } else {
                subtestName = subtestName.replaceAll("%", "");
            }

            return buildInternal();
        }
    }

    public String getLowerTestName() {
        return testName.toLowerCase(Locale.ROOT);
    }

    public String getLowerSubtestName() {
        return subtestName.toLowerCase(Locale.ROOT);
    }
}
