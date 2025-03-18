package ru.yandex.ci.storage.tests;

import java.util.List;
import java.util.Set;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;

@Slf4j
public class TestMetricsServiceImpl implements TestMetricsService {
    @Override
    public void insert(List<TestMetricEntity> metrics) {
        log.info("Inserting {} test metrics", metrics.size());
    }

    @Override
    public Set<String> getTestMetrics(TestStatusEntity.Id statusId) {
        return Set.of();
    }

    @Override
    public List<TestMetricEntity> getMetricHistory(
            TestStatusEntity.Id statusId, String metricName, StorageFrontHistoryApi.TestMetricHistoryFilters filters
    ) {
        return List.of();
    }
}
