package ru.yandex.ci.logbroker;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import io.micrometer.core.instrument.binder.jvm.ExecutorServiceMetrics;
import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;

import ru.yandex.ci.metrics.CommonMetricNames;

public class LogbrokerStatisticsImpl implements LogbrokerStatistics {
    protected final String metricsPrefix;
    protected final String stream;
    protected final MeterRegistry meterRegistry;

    protected final Counter compressedBytes;
    protected final Counter decompressedBytes;

    protected final Map<String, AtomicInteger> lockedPartitionsByTopic = new ConcurrentHashMap<>();
    protected final AtomicInteger lockedPartitionsAll = new AtomicInteger();

    protected final AtomicInteger notCommitedReads = new AtomicInteger();

    protected final Counter readReceived;
    protected final Counter readProcessed;
    protected final Counter readFailed;
    protected final Counter readCommited;

    protected final Counter readFailedAsStorageError;
    protected final Counter commitFailed;

    private final Histogram readDrainAmount;

    private final Map<Integer, Counter> compressedBytesByPartition = new ConcurrentHashMap<>();
    private final Map<Integer, Counter> decompressedBytesByPartition = new ConcurrentHashMap<>();

    public LogbrokerStatisticsImpl(
            String metricsPrefix, String stream, MeterRegistry meterRegistry, CollectorRegistry collectorRegistry
    ) {
        this.metricsPrefix = metricsPrefix;
        this.stream = stream;
        this.meterRegistry = meterRegistry;

        Gauge.builder(metricsPrefix + stream + "_lb_not_commited_reads", notCommitedReads::get)
                .register(meterRegistry);

        Gauge.builder(metricsPrefix + stream + "_lb_locked_partitions", lockedPartitionsAll::get)
                .tag("topic", "all")
                .register(meterRegistry);

        this.compressedBytes = Counter.builder(metricsPrefix + stream + "_lb_bytes")
                .tag("type", "compressed")
                .tag("partition", "all")
                .register(meterRegistry);

        this.decompressedBytes = Counter.builder(metricsPrefix + stream + "_lb_bytes")
                .tag("type", "decompressed")
                .tag("partition", "all")
                .register(meterRegistry);

        this.readReceived = Counter
                .builder(metricsPrefix + stream + "_lb_read")
                .tag("counter", "received")
                .register(meterRegistry);

        this.readProcessed = Counter
                .builder(metricsPrefix + stream + "_lb_read")
                .tag("counter", "processed")
                .register(meterRegistry);

        this.readFailed = Counter
                .builder(metricsPrefix + stream + "_lb_read")
                .tag("counter", "failed")
                .register(meterRegistry);

        this.readFailedAsStorageError = Counter
                .builder(metricsPrefix + CommonMetricNames.ERRORS)
                .tag(metricsPrefix + CommonMetricNames.ERROR_TYPE, stream + "_lb_read")
                .tag(metricsPrefix + CommonMetricNames.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.readCommited = Counter
                .builder(metricsPrefix + stream + "_lb_read")
                .tag("counter", "commited")
                .register(meterRegistry);

        this.commitFailed = Counter
                .builder(metricsPrefix + CommonMetricNames.ERRORS)
                .tag(metricsPrefix + CommonMetricNames.ERROR_TYPE, stream + "_lb_commit")
                .tag(metricsPrefix + CommonMetricNames.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.readDrainAmount = Histogram.build()
                .name(metricsPrefix + stream + "_lb_stream_queue_drain")
                .help("Queue drain amount")
                .buckets(new double[]{1, 2, 4, 8, 16, 32})
                .register(collectorRegistry);
    }

    @Override
    public void onDataReceived(int partition, int compressedLength, int decompressedLength) {
        this.compressedBytes.increment(compressedLength);
        this.decompressedBytes.increment(decompressedLength);

        compressedBytesByPartition.computeIfAbsent(
                partition,
                p -> Counter.builder(metricsPrefix + "lb_bytes")
                        .tag("type", "compressed")
                        .tag("partition", String.valueOf(p))
                        .register(meterRegistry)
        ).increment(compressedLength);

        decompressedBytesByPartition.computeIfAbsent(
                partition,
                p -> Counter.builder(metricsPrefix + stream + "_lb_bytes")
                        .tag("type", "decompressed")
                        .tag("partition", String.valueOf(p))
                        .register(meterRegistry)
        ).increment(compressedLength);
    }

    @Override
    public void onReadsCommited(int number) {
        this.readCommited.increment(number);
        this.notCommitedReads.addAndGet(-number);
    }

    @Override
    public void onReadReceived() {
        this.readReceived.increment();
        this.notCommitedReads.incrementAndGet();
    }

    @Override
    public void onReadFailed() {
        this.readFailed.increment();
        this.readFailedAsStorageError.increment();
    }

    @Override
    public void onReadProcessed() {
        this.readProcessed.increment();
    }

    @Override
    public void onPartitionLocked(String topic) {
        var counter = lockedPartitionsByTopic.computeIfAbsent(topic, (t) -> {
            var value = new AtomicInteger();
            Gauge.builder(metricsPrefix + stream + "_lb_locked_partitions", value::get)
                    .tag("topic", topic)
                    .register(meterRegistry);
            return value;
        });

        counter.incrementAndGet();
        this.lockedPartitionsAll.incrementAndGet();
    }

    @Override
    public void onPartitionUnlocked(String topic) {
        this.lockedPartitionsByTopic.get(topic).decrementAndGet();
        this.lockedPartitionsAll.decrementAndGet();
    }

    @Override
    public void onReadQueueDrain(int amount) {
        this.readDrainAmount.observe(amount);
    }

    @Override
    public void onCommitFailed() {
        this.commitFailed.increment();
    }

    @Override
    public void register(Gauge.Builder<?> gauge) {
        gauge.register(meterRegistry);
    }

    public Counter register(Counter.Builder counter) {
        return counter.register(meterRegistry);
    }

    @Override
    public ExecutorService monitor(ExecutorService executorService, String s) {
        return ExecutorServiceMetrics.monitor(
                meterRegistry, executorService, metricsPrefix + "chunk_executor"
        );
    }

    public void monitor(Cache<?, ?> cache, String name) {
        GuavaCacheMetrics.monitor(meterRegistry, cache, name);
    }

    protected void reset() {
        compressedBytesByPartition.clear();
        decompressedBytesByPartition.clear();
        notCommitedReads.set(0);
        lockedPartitionsAll.set(0);
    }
}
