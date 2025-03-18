package ru.yandex.ci.storage.shard;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class ShardStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "shard_in";

    @Delegate
    private final StreamProcessingStatistics processing;

    private final Counter resultsProcessed;
    private final Counter testsUpdated;
    private final Counter aggregatesReported;

    private final Counter logicErrors;
    private final Counter chunkErrors;
    private final Counter postProcessorErrors;
    private final Counter aggregateErrors;
    private final Counter aggregateReportErrors;
    private final Counter aYamlerClientErrors;
    private final Counter bulkInsertErrors;
    private final Counter dbUnavailableError;

    private final Counter numberOfShardOutWrites;

    private final Histogram chunkQueueDrainAmount;
    private final Histogram aggregateQueueDrainAmount;
    private final Histogram aggregatesBatchSize;
    private final Histogram resultsBatchSize;

    private final ConcurrentMap<String, Counter> chunkProcessedResults = new ConcurrentHashMap<>();
    private final Counter negativeStatisticsErrors;

    public ShardStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);

        this.bulkInsertErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "db_bulk_insert")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.dbUnavailableError = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "db_unavailable")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.chunkErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "chunk")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.logicErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "logic")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.postProcessorErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "post_processor")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.negativeStatisticsErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "negative_statistics")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.aggregateErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "aggregate")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.aggregateReportErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "aggregate_report")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.aYamlerClientErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "ayamler_client")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.resultsProcessed = Counter
                .builder(StorageMetrics.RESULTS_PROCESSED)
                .register(meterRegistry);

        this.testsUpdated = Counter
                .builder(StorageMetrics.PREFIX + "tests_updated")
                .register(meterRegistry);

        this.aggregatesReported = Counter
                .builder(StorageMetrics.PREFIX + "aggregates_reported")
                .register(meterRegistry);

        this.numberOfShardOutWrites = Counter.builder(StorageMetrics.PREFIX + "shard_out_writes")
                .register(meterRegistry);

        this.chunkQueueDrainAmount = Histogram.build()
                .name(StorageMetrics.PREFIX + "chunk_queue_drain")
                .help("Queue drain amount")
                .buckets(new double[]{1, 4, 8, 16, 32, 64, 128, 256})
                .register(collectorRegistry);

        this.aggregateQueueDrainAmount = Histogram.build()
                .name(StorageMetrics.PREFIX + "aggregate_queue_drain")
                .help("Queue drain amount")
                .buckets(new double[]{1, 4, 8, 16, 32, 64, 128, 256})
                .register(collectorRegistry);

        this.aggregatesBatchSize = Histogram.build()
                .name(StorageMetrics.PREFIX + "aggregates_batch_size")
                .help("Results batch size")
                .buckets(new double[]{4, 16, 64, 256, 1024, 2048, 4096, 8192, 16384})
                .register(collectorRegistry);

        this.resultsBatchSize = Histogram.build()
                .name(StorageMetrics.PREFIX + "results_batch_size")
                .help("Results batch size")
                .buckets(new double[]{4, 16, 64, 256, 1024, 2048, 4096, 8192, 16384})
                .register(collectorRegistry);
    }

    public void onResultsInserted(int numberOfResults) {
        this.resultsBatchSize.observe(numberOfResults);
    }

    public void onResultsProcessed(ChunkEntity.Id chunkId, int numberOfResults) {
        this.resultsProcessed.increment(numberOfResults);
        this.aggregatesBatchSize.observe(numberOfResults);

        var counter = this.chunkProcessedResults.computeIfAbsent(chunkId.toString(), id ->
                Counter
                        .builder(StorageMetrics.PREFIX + "chunk_results_processed")
                        .tag("chunk", id)
                        .register(meterRegistry));
        counter.increment(numberOfResults);
    }

    public void onChunkWorkerError() {
        this.chunkErrors.increment();
    }

    public void onPostProcessorWorkerError() {
        this.postProcessorErrors.increment();
    }

    public void onChunkQueueDrain(int amount) {
        this.chunkQueueDrainAmount.observe(amount);
    }

    public void onTestsUpdated(int numberOfTests) {
        this.testsUpdated.increment(numberOfTests);
    }

    public void onAggregateWorkerError() {
        this.aggregateErrors.increment();
    }

    public void onAggregateQueueDrain(int amount) {
        this.aggregateQueueDrainAmount.observe(amount);
    }

    public void onAggregateReporterWorkerError() {
        this.aggregateReportErrors.increment();
    }

    public void onAYamlerClientError() {
        this.aYamlerClientErrors.increment();
    }

    public void onBulkInsertError() {
        this.bulkInsertErrors.increment();
    }

    public void onDbUnavailableError() {
        this.dbUnavailableError.increment();
    }


    public void onAggregateReporterDrain(int amount) {
    }

    public void onAggregateReported(int amount) {
        aggregatesReported.increment(amount);
    }

    public void onOutWrite() {
        numberOfShardOutWrites.increment();
    }

    public void onNegativeStatisticsError() {
        this.negativeStatisticsErrors.increment();
    }

    public void onLogicError() {
        this.logicErrors.increment();
    }
}
