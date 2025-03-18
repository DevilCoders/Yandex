package ru.yandex.ci.storage.post_processor.processing;

import java.time.Instant;
import java.util.List;
import java.util.stream.Stream;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.util.Retryable;

@Slf4j
@AllArgsConstructor
public class MetricsProcessor {
    private final TestMetricsService metricsService;
    private final PostProcessorStatistics statistics;

    public void process(List<ResultMessage> messages) {
        var metrics = messages.stream().map(ResultMessage::getResult).flatMap(this::extractMetrics).toList();
        log.info("{} metrics from {} messages", metrics.size(), messages.size());

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> metricsService.insert(metrics),
                r -> this.statistics.onClickhouseInsertError(),
                true
        );

        messages.forEach(message -> message.getCountdown().notifyMessageProcessed());
    }

    private Stream<TestMetricEntity> extractMetrics(PostProcessorTestResult message) {
        return message.getMetrics().entrySet().stream().map(
                x -> TestMetricEntity.builder()
                        .branch(message.getBranch())
                        .path(message.getPath())
                        .testName(message.getName())
                        .subtestName(message.getSubtestName())
                        .toolchain(message.getId().getToolchain())
                        .metricName(x.getKey())
                        .resultType(message.getResultType())
                        .revision(message.getRevisionNumber())
                        .timestamp(Instant.now())
                        .value(x.getValue())
                        .testStatus(message.getStatus())
                        .checkId(message.getId().getCheckId().getId())
                        .suiteId(message.getId().getSuiteId())
                        .testId(message.getId().getTestId())
                        .build()
        );
    }
}
