package ru.yandex.ci.storage.core.test_metrics;

import java.util.List;
import java.util.Set;

import lombok.AllArgsConstructor;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricTable;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

@AllArgsConstructor
public class TestMetricsServiceImpl implements TestMetricsService {
    private final TestMetricTable testMetricTable;
    private final CiStorageDb db;

    @Override
    public void insert(List<TestMetricEntity> metrics) {
        testMetricTable.save(metrics);
    }

    @Override
    public Set<String> getTestMetrics(TestStatusEntity.Id statusId) {
        var toolchains = db.currentOrReadOnly(
                () -> db.tests().find(statusId.getTestId(), statusId.getSuiteId(), statusId.getBranch())
        );

        if (toolchains.isEmpty()) {
            throw GrpcUtils.notFoundException("Test not found" + statusId.toString());
        }

        return testMetricTable.getTestMetrics(toolchains.get(0));
    }

    @Override
    public List<TestMetricEntity> getMetricHistory(
            TestStatusEntity.Id statusId, String metricName, StorageFrontHistoryApi.TestMetricHistoryFilters filters
    ) {
        var toolchains = db.currentOrReadOnly(
                () -> db.tests().find(statusId.getTestId(), statusId.getSuiteId(), statusId.getBranch())
        );

        if (toolchains.isEmpty()) {
            throw GrpcUtils.notFoundException("Test not found" + statusId.toString());
        }

        return testMetricTable.getMetricHistory(toolchains.get(0), metricName, filters);
    }
}
