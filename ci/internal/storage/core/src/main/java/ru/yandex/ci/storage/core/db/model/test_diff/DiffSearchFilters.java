package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.Set;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
public class DiffSearchFilters {
    Set<Common.ResultType> resultTypes;
    Set<Common.TestDiffType> diffTypes;

    String toolchain;
    String path;
    Set<String> pathIn; // Direct paths we can use in `in` queries
    String name;
    String subtestName;

    Set<String> tags;

    StorageFrontApi.NotificationFilter notificationFilter;

    Integer page;

    public static class Builder {
        public DiffSearchFilters build() {
            if (resultTypes == null) {
                resultTypes = Set.of();
            }

            if (diffTypes == null) {
                diffTypes = Set.of();
            }

            if (toolchain == null) {
                toolchain = TestEntity.ALL_TOOLCHAINS;
            }

            if (path == null) {
                path = "";
            } else {
                path = path.replaceAll("%", "");
            }

            if (pathIn == null) {
                pathIn = Set.of();
            }

            if (name == null) {
                name = "";
            } else {
                name = name.replaceAll("%", "");
            }

            if (subtestName == null) {
                subtestName = "";
            } else {
                subtestName = subtestName.replaceAll("%", "");
            }

            if (tags == null) {
                tags = Set.of();
            }

            if (notificationFilter == null) {
                notificationFilter = StorageFrontApi.NotificationFilter.NF_NONE;
            }

            if (page == null || page < 1) {
                page = 1;
            }

            return buildInternal();
        }
    }
}
