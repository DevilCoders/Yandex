package ru.yandex.ci.ydb.service.metric;

import java.time.Instant;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class MetricTable extends KikimrTableCi<Metric> {

    public MetricTable(QueryExecutor executor) {
        super(Metric.class, executor);
    }

    public Optional<Metric> getLastMetric(MetricId metricId) {
        return find(
                YqlPredicate.where("id.id").eq(metricId.asString()),
                YqlOrderBy.orderBy("id.time", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ).stream().findFirst();
    }

    public void save(MetricId metricId, double value) {
        save(metricId, value, Instant.now());
    }

    public void save(MetricId metricId, double value, Instant time) {
        save(Metric.of(metricId, time, value));
    }
}
