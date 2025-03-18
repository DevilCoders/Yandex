package ru.yandex.ci.observer.reader.message.events;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.EventsStreamMessages.EventsStreamMessage.MessagesCase;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

/**
 * Process traces from storage-reader (i.e. registration, processing finish)
 */
@Slf4j
public class ObserverEventsStreamReadProcessor implements LogbrokerReadProcessor {
    private final EventsStreamStatistics statistics;
    private final ObserverEventsStreamQueuedReadProcessor queuedReadProcessor;
    private final Map<MessagesCase, Consumer<List<ReadEventStreamMessage>>> messageCasesToProcess = Map.of(
            MessagesCase.REGISTRATION, this::processRegistrationMessages,
            MessagesCase.CANCEL, this::processCancelMessages,
            MessagesCase.TRACE, this::processTraceMessages,
            MessagesCase.ITERATION_FINISH, this::processIterationFinishMessages
    );

    public ObserverEventsStreamReadProcessor(EventsStreamStatistics statistics,
                                             ObserverEventsStreamQueuedReadProcessor queuedReadProcessor) {
        this.statistics = statistics;
        this.queuedReadProcessor = queuedReadProcessor;
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        var parsedMessages = reads.stream()
                .flatMap(r -> r.getBatches().stream()
                        .flatMap(x -> x.getMessageData().stream()
                                .map(messageData -> parseWithRetries(x.getPartition(), messageData)))
                        .filter(Optional::isPresent)
                        .map(optional -> optional.orElseThrow(RuntimeException::new))
                        .map(m -> new ReadEventStreamMessage(r, m))
                )
                .collect(Collectors.toList());

        if (parsedMessages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            reads.forEach(LogbrokerPartitionRead::notifyProcessed);
            return;
        }

        LogbrokerLogger.logReceived(
                parsedMessages,
                m -> m.getMessage().getMeta()
        );

        var messagesByCase = parsedMessages.stream().collect(
                Collectors.groupingBy(m -> m.getMessage().getMessagesCase())
        );

        log.info(
                "Processing events logbroker reads:, " +
                        "reads: {} partitions: {}, lb messages: {}, messages: {}, by type: [{}], cookies: [{}]",
                reads.size(),
                reads.stream().map(read -> String.valueOf(read.getPartition())).collect(Collectors.joining(", ")),
                reads.stream().mapToInt(LogbrokerPartitionRead::getNumberOfMessages).sum(),
                parsedMessages.size(),
                NumberOfMessagesFormatter.format(messagesByCase),
                reads.stream()
                        .map(read -> String.valueOf(read.getCommitCountdown().getCookie()))
                        .collect(Collectors.joining(", "))
        );

        messageCasesToProcess.forEach((c, consumer) -> consumer.accept(messagesByCase.getOrDefault(c, List.of())));

        messagesByCase.forEach((c, l) -> {
            if (messageCasesToProcess.containsKey(c)) {
                return;
            }

            l.forEach(r -> r.getRead().getCommitCountdown().notifyMessageProcessed(1));
        });

        log.info("All messages enqueued to processing");
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking events stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking events stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }

    private void processTraceMessages(List<ReadEventStreamMessage> messages) {
        var byCheck = messages.stream()
                .collect(
                        Collectors.groupingBy(
                                m -> CheckEntity.Id.of(
                                        m.getMessage().getTrace().getFullTaskId().getIterationId().getCheckId()
                                ),
                                Collectors.toList()
                        )
                );

        byCheck.forEach(
                (c, list) -> list.forEach(m -> queuedReadProcessor.enqueue(getQueueNumber(c), m))
        );
    }

    private void processCancelMessages(List<ReadEventStreamMessage> messages) {
        var byCheck = messages.stream()
                .collect(
                        Collectors.groupingBy(
                                m -> CheckEntity.Id.of(m.getMessage().getCancel().getIterationId().getCheckId()),
                                Collectors.toList()
                        )
                );

        byCheck.forEach(
                (c, list) -> list.forEach(m -> queuedReadProcessor.enqueue(getQueueNumber(c), m))
        );
    }

    private void processRegistrationMessages(List<ReadEventStreamMessage> messages) {
        var byCheck = messages.stream()
                .collect(
                        Collectors.groupingBy(
                                m -> CheckEntity.Id.of(m.getMessage().getRegistration().getCheck().getId()),
                                Collectors.toList()
                        )
                );

        byCheck.forEach(
                (c, list) -> list.forEach(m -> queuedReadProcessor.enqueue(getQueueNumber(c), m))
        );
    }

    private void processIterationFinishMessages(List<ReadEventStreamMessage> messages) {
        var byCheck = messages.stream()
                .collect(
                        Collectors.groupingBy(
                                m -> CheckEntity.Id.of(
                                        m.getMessage().getIterationFinish().getIterationId().getCheckId()
                                ),
                                Collectors.toList()
                        )
                );

        byCheck.forEach(
                (c, list) -> list.forEach(m -> queuedReadProcessor.enqueue(getQueueNumber(c), m))
        );
    }

    private boolean skipParseErrors() {
        return true;
    }

    protected Optional<EventsStreamMessages.EventsStreamMessage> parseWithRetries(
            int partition, MessageData messageData
    ) {
        var retryDurationSeconds = Retryable.START_TIMEOUT_SECONDS;
        var retryDurationSecondsMax = Retryable.MAX_TIMEOUT_SECONDS;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(parseMessage(partition, messageData));
            } catch (RuntimeException | IOException e) {
                log.error("Failed to process message", e);
                this.statistics.onParseError();

                if (this.skipParseErrors()) {
                    log.info("Skipping parse error");
                    return Optional.empty();
                }

                Uninterruptibles.sleepUninterruptibly(retryDurationSeconds, TimeUnit.SECONDS);
                retryDurationSeconds = Math.min(retryDurationSecondsMax, retryDurationSeconds * 2);
            }
        }

        return Optional.empty();
    }

    private int getQueueNumber(CheckEntity.Id checkId) {
        return (checkId.hashCode() & 0xfffffff) % queuedReadProcessor.getQueueMaxNumber();
    }

    private EventsStreamMessages.EventsStreamMessage parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.onDataReceived(partition, 0, decompressed.length);
        return EventsStreamMessages.EventsStreamMessage.parseFrom(decompressed);
    }
}
