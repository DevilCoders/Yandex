package ru.yandex.ci.storage.post_processor;

import java.time.Clock;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.NavigableMap;
import java.util.Set;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class PostProcessorStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "post_processor_in";

    @Delegate
    private final StreamProcessingStatistics processing;

    private final Clock clock;

    private final Counter bulkInsertErrors;
    private final Counter dbUnavailableError;
    private final Counter postProcessorWorkerErrors;
    private final Counter testsUpdated;

    private final Counter historyUnchangedProcessed;
    private final Counter historyChangedProcessed;

    private final Histogram messageQueueDrainAmount;

    private final Counter regionLoad;
    private final Counter bucketFromDbLoad;
    private final Counter bucketFromCacheLoad;
    private final Counter bucketEmptyLoad;
    private final Counter historyErrors;
    private final Counter clickhouseInsertErrors;

    protected final AtomicInteger numberOfRegions = new AtomicInteger();
    protected final AtomicInteger numberOfBuckets = new AtomicInteger();
    protected final AtomicInteger numberOfRevisions = new AtomicInteger();

    protected final AtomicReference<Instant> lastObserverCleanUp;
    protected final NavigableMap<Long, Instant> observedRevisions;

    public PostProcessorStatistics(
            MeterRegistry meterRegistry, CollectorRegistry collectorRegistry, Clock clock
    ) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.clock = clock;
        this.lastObserverCleanUp = new AtomicReference<>(clock.instant());
        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);

        this.bulkInsertErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "db_bulk_insert")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.clickhouseInsertErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "db_clickhouse_insert")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.dbUnavailableError = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "db_unavailable")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.postProcessorWorkerErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "message_processor_worker")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.historyErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "history_processing")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.testsUpdated = Counter
                .builder(StorageMetrics.PREFIX + "tests_updated")
                .register(meterRegistry);

        this.messageQueueDrainAmount = Histogram.build()
                .name(StorageMetrics.PREFIX + "message_queue_drain")
                .help("Queue drain amount")
                .buckets(new double[]{1, 4, 8, 32, 128, 512, 1024})
                .register(collectorRegistry);

        var historyMetric = StorageMetrics.PREFIX + "history_processed";
        this.historyUnchangedProcessed = Counter.builder(historyMetric).tag("changed", "no").register(meterRegistry);
        this.historyChangedProcessed = Counter.builder(historyMetric).tag("changed", "yes").register(meterRegistry);

        this.regionLoad = Counter.builder(StorageMetrics.PREFIX + "region_load").register(meterRegistry);

        var bucketLoadMetric = StorageMetrics.PREFIX + "bucket_load";
        this.bucketFromDbLoad = Counter.builder(bucketLoadMetric).tag("from", "db").register(meterRegistry);
        this.bucketFromCacheLoad = Counter.builder(bucketLoadMetric).tag("from", "cache").register(meterRegistry);
        this.bucketEmptyLoad = Counter.builder(bucketLoadMetric).tag("from", "empty").register(meterRegistry);
        this.observedRevisions = new ConcurrentSkipListMap<>();

        Gauge.builder(StorageMetrics.PREFIX + "number_of_regions", numberOfRegions::get)
                .register(meterRegistry);

        Gauge.builder(StorageMetrics.PREFIX + "number_of_buckets", numberOfBuckets::get)
                .register(meterRegistry);

        Gauge.builder(StorageMetrics.PREFIX + "number_of_revisions", numberOfRevisions::get)
                .register(meterRegistry);

        Gauge.builder(StorageMetrics.PREFIX + "observed_revisions", observedRevisions::lastKey)
                .tag("key", "high")
                .register(meterRegistry);

        Gauge.builder(StorageMetrics.PREFIX + "observed_revisions", observedRevisions::firstKey)
                .tag("key", "low")
                .register(meterRegistry);

        Gauge.builder(StorageMetrics.PREFIX + "observed_revisions_count", observedRevisions::size)
                .register(meterRegistry);
    }

    public void onBulkInsertError() {
        this.bulkInsertErrors.increment();
    }

    public void onDbUnavailableError() {
        this.dbUnavailableError.increment();
    }

    public void onPostProcessorWorkerError() {
        this.postProcessorWorkerErrors.increment();
    }

    public void onTestsUpdated(int amount) {
        this.testsUpdated.increment(amount);
    }

    public void onMessageQueueDrain(int amount) {
        this.messageQueueDrainAmount.observe(amount);
    }

    public void onBucketFromDbLoad() {
        this.bucketFromDbLoad.increment();
    }

    public void onBucketEmptyLoad() {
        this.bucketEmptyLoad.increment();
    }

    public void onBucketFromCacheLoad() {
        this.bucketFromCacheLoad.increment();
    }

    public void onRegionLoad() {
        this.regionLoad.increment();
    }

    public void onHistoryUnchangedProcessed(int amount) {
        this.historyUnchangedProcessed.increment(amount);
    }

    public void onHistoryChangedProcessed(int amount) {
        this.historyChangedProcessed.increment(amount);
    }

    public void onHistoryError() {
        this.historyErrors.increment();
    }

    public void onClickhouseInsertError() {
        this.clickhouseInsertErrors.increment();
    }

    public void onRegionCreated() {
        this.numberOfRegions.incrementAndGet();
    }

    public void onBucketCreated() {
        this.numberOfBuckets.incrementAndGet();
    }

    public void onRevisionsAdded(int number) {
        this.numberOfRevisions.addAndGet(number);
    }

    public int getNumberOfRegions() {
        return numberOfRegions.get();
    }

    public int getNumberOfBuckets() {
        return numberOfBuckets.get();
    }

    public int getNumberOfRevisions() {
        return numberOfRevisions.get();
    }

    public void onRegionRemoved() {
        this.numberOfRegions.decrementAndGet();
    }

    public void onBucketRemoved() {
        this.numberOfBuckets.decrementAndGet();
    }

    public void onRevisionsRemoved(int number) {
        this.numberOfRevisions.addAndGet(-number);
    }

    public void onReceivedRevisions(Set<Long> revisions) {
        var now = clock.instant();
        revisions.forEach(number -> this.observedRevisions.put(number, now));

        var lastClean = lastObserverCleanUp.get();
        if (lastClean.isBefore(now.minus(1, ChronoUnit.MINUTES))) {
            if (lastObserverCleanUp.compareAndSet(lastClean, now)) {
                var cut = now.minus(10, ChronoUnit.MINUTES);
                this.observedRevisions.entrySet().removeIf(x -> x.getValue().isBefore(cut));
            }
        }
    }

    public void reset() {
        super.reset();
        this.numberOfRegions.set(0);
        this.numberOfBuckets.set(0);
        this.numberOfRevisions.set(0);
    }
}
