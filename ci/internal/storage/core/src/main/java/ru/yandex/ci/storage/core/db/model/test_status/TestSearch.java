package ru.yandex.ci.storage.core.db.model.test_status;

import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;

@Value
@Builder
public class TestSearch {
    String branch;
    String project;
    String path;
    String name;
    String subtestName;
    Set<Common.ResultType> resultTypes;
    Set<Common.TestStatus> statuses;
    StorageFrontApi.NotificationFilter notificationFilter;

    Paging paging;

    @Value
    public static class Page {
        String path;
        long testId;
    }

    @Value
    @lombok.Builder
    public static class Paging {
        Page page;
        boolean ascending;
        int pageSize;
    }

    @Value
    public static class Results {
        Map<TestStatusEntity.Id, List<TestStatusEntity>> results;

        @Nullable
        Page nextPage;

        @Nullable
        Page previousPage;
    }
}
