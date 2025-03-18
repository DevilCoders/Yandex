package ru.yandex.ci.storage.core.logbroker;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

@Slf4j
public abstract class StorageWriterBase<DATA, MESSAGE extends StorageWriterBase.Message<DATA>> {

    @Nonnull
    private final LogbrokerWriterFactory logbrokerWriterFactory;

    protected final int numberOfPartitions;

    @Nonnull
    protected final Statistics statistics;

    @Nonnull
    protected final ConcurrentMap<Integer, LogbrokerWriter> partitionProducers;

    public StorageWriterBase(
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        this.numberOfPartitions = numberOfPartitions;
        this.partitionProducers = new ConcurrentHashMap<>(numberOfPartitions);
        this.statistics = new Statistics(meterRegistry, numberOfPartitions, logbrokerWriterFactory.getStreamName());
        this.logbrokerWriterFactory = logbrokerWriterFactory;
    }

    protected abstract CompletableFuture<ProducerWriteResponse> write(
            int partition, LogbrokerWriter writer, DATA messageData
    );

    protected List<CompletableFuture<ProducerWriteResponse>> writeWithoutRetries(List<MESSAGE> messages) {
        if (messages.isEmpty()) {
            return List.of();
        }

        var writes = new ArrayList<CompletableFuture<ProducerWriteResponse>>(messages.size());
        for (var message : messages) {
            var partition = message.getLogbrokerPartition();
            var producer = partitionProducers.computeIfAbsent(partition, this::createWriter);

            writes.add(write(partition, producer, message.getMessage()));
        }

        this.statistics.pendingWrites.addAndGet(writes.size());

        return writes.stream()
                .map(write -> write.whenComplete((result, ex) -> {
                    this.statistics.pendingWrites.decrementAndGet();
                    if (ex != null) {
                        log.warn("Failed to write to {}", getStreamName(), ex);
                        this.statistics.onWriteFailed();
                    }
                }))
                .collect(Collectors.toList());
    }

    protected void writeWithRetries(List<MESSAGE> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var notCompletedMessages = messages;

        var retryTimeoutSeconds = Retryable.START_TIMEOUT_SECONDS;
        var isFirstTry = true;
        while (true) {
            notCompletedMessages = writeInternal(notCompletedMessages, isFirstTry);
            if (notCompletedMessages.isEmpty()) {
                break;
            }

            isFirstTry = false;

            log.warn("Not completed messages: {}", notCompletedMessages.size());

            try {
                log.info("Sleep before retry: {}s", retryTimeoutSeconds);
                Thread.sleep(TimeUnit.SECONDS.toMillis(retryTimeoutSeconds));
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            }

            retryTimeoutSeconds = Math.min(retryTimeoutSeconds * 2, Retryable.MAX_TIMEOUT_SECONDS);
        }
    }

    private List<MESSAGE> writeInternal(List<MESSAGE> messages, boolean isFirstTry) {
        if (messages.isEmpty()) {
            return List.of();
        }

        var writes = new ArrayList<CompletableFuture<ProducerWriteResponse>>(messages.size());
        var producers = new ArrayList<LogbrokerWriter>(messages.size());
        for (var message : messages) {
            var partition = Math.max(0, message.getLogbrokerPartition());
            var producer = partitionProducers.computeIfAbsent(partition, this::createWriter);

            producers.add(producer);
            writes.add(write(partition, producer, message.getMessage()));
        }

        var notCompletedMessages = new ArrayList<MESSAGE>(messages.size());

        for (var i = 0; i < writes.size(); i++) {
            try {
                var result = writes.get(i).get();
                var message = messages.get(i);
                var producer = producers.get(i);

                LogbrokerLogger.logSend(result, message, producer);
            } catch (InterruptedException e) {
                throw new RuntimeException("Interrupted", e);
            } catch (ExecutionException e) {
                notCompletedMessages.add(messages.get(i));
                if (isFirstTry) {
                    log.warn("Failed to write.", e);
                } else {
                    this.statistics.onWriteFailed();
                    log.error("Failed to write.", e);
                }
            }
        }

        return notCompletedMessages;
    }

    protected LogbrokerWriter createWriter(int partition) {
        return logbrokerWriterFactory.create(partition);
    }

    private String getStreamName() {
        return logbrokerWriterFactory.getStreamName();
    }

    protected static class Statistics {
        private final Counter numberOfFailedWrites;
        private final Counter numberOfWrites;
        private final Counter totalWrittenBytes;
        private final Map<Integer, Counter> writtenBytes = new HashMap<>();

        private final AtomicInteger pendingWrites = new AtomicInteger();

        private Statistics(MeterRegistry meterRegistry, int numberOfPartitions, String streamName) {
            var bytesWrittenName = StorageMetrics.PREFIX + "bytes_written_to_" + streamName;
            for (var i = 0; i < numberOfPartitions; ++i) {
                writtenBytes.put(
                        i,
                        Counter.builder(bytesWrittenName)
                                .tag("partition", String.valueOf(i))
                                .register(meterRegistry)
                );
            }

            Gauge.builder(StorageMetrics.PREFIX + streamName + "_pending_writes", pendingWrites::get)
                    .register(meterRegistry);

            this.totalWrittenBytes = Counter.builder(bytesWrittenName)
                    .tag("partition", "all")
                    .register(meterRegistry);

            this.numberOfWrites = Counter.builder(StorageMetrics.PREFIX + "writes_to_" + streamName)
                    .register(meterRegistry);

            this.numberOfFailedWrites = Counter.builder(StorageMetrics.ERRORS)
                    .tag(StorageMetrics.ERROR_TYPE, streamName + "_write")
                    .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                    .register(meterRegistry);
        }

        public void onWrite(int partition, int bytes) {
            this.numberOfWrites.increment();
            this.writtenBytes.get(partition).increment(bytes);
            this.totalWrittenBytes.increment(bytes);
        }

        public void onWriteFailed() {
            this.numberOfFailedWrites.increment();
        }
    }

    @Getter
    @AllArgsConstructor
    protected static class Message<DATA> {
        private final int logbrokerPartition;
        private final DATA message;

        public String toLogString() {
            return "class: %s, partition: %d".formatted(
                    getMessage().getClass().getSimpleName(), getLogbrokerPartition()
            );
        }
    }
}
