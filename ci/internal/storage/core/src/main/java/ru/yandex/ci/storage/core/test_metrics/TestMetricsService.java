package ru.yandex.ci.storage.core.test_metrics;

import java.util.List;
import java.util.Set;

import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public interface TestMetricsService {
    void insert(List<TestMetricEntity> metrics);

    Set<String> getTestMetrics(TestStatusEntity.Id statusId);

    List<TestMetricEntity> getMetricHistory(
            TestStatusEntity.Id statusId, String metricName, StorageFrontHistoryApi.TestMetricHistoryFilters filters
    );
}
