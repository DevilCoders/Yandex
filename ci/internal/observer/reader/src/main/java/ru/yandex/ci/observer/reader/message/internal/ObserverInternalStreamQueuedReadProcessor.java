package ru.yandex.ci.observer.reader.message.internal;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.Gauge;
import lombok.Getter;

import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

public class ObserverInternalStreamQueuedReadProcessor {
    private static final String NAME = "internal-queued-processor";

    private final InternalStreamStatistics statistics;
    private final ObserverInternalStreamMessageProcessor processor;
    private final int drainLimit;
    @Getter
    private final int queueMaxNumber;
    private final QueueExecutor<ReadInternalStreamMessage> executor;

    public ObserverInternalStreamQueuedReadProcessor(
            InternalStreamStatistics statistics,
            ObserverInternalStreamMessageProcessor processor,
            int drainLimit, int queueMaxNumber, boolean syncMode
    ) {
        this.executor = syncMode ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>(NAME, this::createQueueWorker);

        this.statistics = statistics;
        this.processor = processor;

        this.drainLimit = drainLimit;
        this.queueMaxNumber = queueMaxNumber;

        this.statistics.register(
                Gauge.builder(NAME + "-queue_size", this.executor::getQueueSize)
                        .tag(StorageMetrics.QUEUE, "all")
        );
    }

    public void enqueue(int queueNumber, ReadInternalStreamMessage message) {
        this.executor.enqueue(String.valueOf(queueNumber), message);
    }

    private QueueWorker<ReadInternalStreamMessage> createQueueWorker(
            String queueName, LinkedBlockingQueue<ReadInternalStreamMessage> queue
    ) {
        return new Worker(queue, drainLimit);
    }

    @VisibleForTesting
    void process(List<ReadInternalStreamMessage> messages) {
        var byCase = messages.stream()
                .collect(Collectors.groupingBy(
                        m -> m.getMessage().getMessagesCase(),
                        Collectors.mapping(ReadInternalStreamMessage::getMessage, Collectors.toList())
                ));

        this.processRegisteredMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.REGISTERED, List.of())
        );
        this.processFatalErrorMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.FATAL_ERROR, List.of())
        );
        this.processCancelMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.CANCEL, List.of())
        );
        this.processPessimizeMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.PESSIMIZE, List.of())
        );
        // TODO process metrics messages
        this.processTechnicalStatsMessages(
                byCase.getOrDefault(
                        Internal.InternalMessage.MessagesCase.TECHNICAL_STATS_MESSAGE, List.of()
                )
        );
        this.processFinishPartitionMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.FINISH_PARTITION, List.of())
        );
        this.processAggregateMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.AGGREGATE, List.of())
        );
        this.processIterationFinishMessages(
                byCase.getOrDefault(Internal.InternalMessage.MessagesCase.ITERATION_FINISH, List.of())
        );

        byCase.forEach((c, m) -> statistics.onMessageProcessed(c, m.size()));
        messages.forEach(m -> m.notifyMessageProcessed(1));
    }

    private void processRegisteredMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processRegisteredMessages(
                messages.stream().map(Internal.InternalMessage::getRegistered).collect(Collectors.toList())
        );
    }

    private void processAggregateMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processAggregateMessages(
                messages.stream().collect(Collectors.groupingBy(
                        m -> m.getAggregate().getIterationId(),
                        Collectors.mapping(Internal.InternalMessage::getAggregate, Collectors.toList())
                ))
        );
    }

    private void processFatalErrorMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processFatalErrorMessages(messages);
    }

    private void processCancelMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processCancelMessages(messages);
    }

    private void processFinishPartitionMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var messagesByTaskIds = messages.stream()
                .map(Internal.InternalMessage::getFinishPartition)
                .collect(Collectors.groupingBy(
                        Internal.FinishPartition::getTaskId,
                        Collectors.toList()
                ));

        processor.processFinishPartitionMessages(messagesByTaskIds);
    }

    private void processPessimizeMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processPessimizeMessages(
                messages.stream().map(m -> m.getPessimize().getIterationId()).distinct().collect(Collectors.toList())
        );
    }

    private void processTechnicalStatsMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        processor.processTechnicalStatsMessages(
                messages.stream().map(Internal.InternalMessage::getTechnicalStatsMessage).collect(Collectors.toList())
        );
    }

    private void processIterationFinishMessages(List<Internal.InternalMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var iterationsToFinish = messages.stream()
                .map(Internal.InternalMessage::getIterationFinish)
                .collect(Collectors.toMap(
                        Internal.IterationFinish::getIterationId,
                        Function.identity(),
                        (s1, s2) -> s1
                ));

        processor.processIterationFinishMessages(iterationsToFinish);
    }

    class Worker extends QueueWorker<ReadInternalStreamMessage> {
        Worker(BlockingQueue<ReadInternalStreamMessage> queue, int drainLimit) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
        }

        @Override
        public void process(List<ReadInternalStreamMessage> messages) {
            ObserverInternalStreamQueuedReadProcessor.this.process(messages);
        }

        @Override
        public void onFailed() {
        }
    }
}
