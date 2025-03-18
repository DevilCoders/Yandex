package ru.yandex.ci.storage.core.db.clickhouse.metrics;

import java.time.Instant;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;

@Value
@AllArgsConstructor
@Builder
public class TestMetricEntity {
    String branch;
    String path;
    String testName;
    String subtestName;
    String toolchain;
    String metricName;
    Instant timestamp;
    Long revision;
    Common.ResultType resultType;
    Double value;
    Common.TestStatus testStatus;
    Long checkId;
    Long suiteId;
    Long testId;
}
